#include <linux/list.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include "broker.h"

static int max_messages = 5;
static int max_message_size = 250;
static broker_s my_broker;

// Função auxiliar para contar mensagens em uma fila, usada para o buffer circular
static int subscriber_message_count(struct list_head *queue) {
    int count = 0;
    struct list_head *entry;
    list_for_each(entry, queue) {
        count++;
    }
    return count;
}

void broker_init(void) {
    INIT_LIST_HEAD(&my_broker.topics);
    printk(KERN_INFO "Broker initialized.\n");
}

topic_s *broker_create_topic(int id) {

    topic_s *new_topic = kmalloc(sizeof(*new_topic), GFP_KERNEL);

    if (!new_topic) {
        printk(KERN_ERR "Failed to allocate memory for new topic.\n");
        return NULL;
    }

    new_topic->id = id;
    INIT_LIST_HEAD(&new_topic->subscribers);

    list_add_tail(&new_topic->topic_node, &my_broker.topics);

    printk(KERN_INFO "Topic %d created.\n", id);
    return new_topic;
}

topic_s *broker_find_topic(int id) {

    topic_s *entry;
    list_for_each_entry(entry, &my_broker.topics, topic_node) {
        if (entry->id == id) {
            return entry;
        }
    }
    return NULL;
}

process_s *topic_find_subscriber(topic_s *topic, int pid) {
    process_s *entry;
    list_for_each_entry(entry, &topic->subscribers, subscriber_node) {
        if (entry->pid == pid) {
            return entry;
        }
    }
    return NULL;
}

int topic_add_subscriber(topic_s *topic, int pid) {
    if (topic_find_subscriber(topic, pid) != NULL) {
        printk(KERN_INFO "PID %d is already a subscriber to topic %d.\n", pid, topic->id);
        return 0;
    }

    process_s *new_process = kmalloc(sizeof(*new_process), GFP_KERNEL);
    if (!new_process) {
        printk(KERN_ERR "Failed to allocate memory for new subscriber.\n");
        return -ENOMEM;
    }

    new_process->pid = pid;
    INIT_LIST_HEAD(&new_process->message_queue);
    list_add_tail(&new_process->subscriber_node, &topic->subscribers);

    printk(KERN_INFO "PID %d added as a subscriber to topic %d.\n", pid, topic->id);
    return 0;
}

int topic_publish_message(topic_s *topic, const char *message_data, short data_size) {
    if (data_size > max_message_size) {
        printk(KERN_ALERT "PubSub Driver: Message too large, discarded.\n");
        return -EINVAL;
    }
    
    process_s *subscriber;
    list_for_each_entry(subscriber, &topic->subscribers, subscriber_node) {
        message_s *new_msg = kmalloc(sizeof(*new_msg), GFP_KERNEL);
        if (!new_msg) {
            printk(KERN_ERR "Failed to allocate memory for new message for PID %d.\n", subscriber->pid);
            continue;
        }

        strncpy(new_msg->message, message_data, min_t(short, max_message_size, data_size));
        new_msg->message[min_t(short, max_message_size, data_size)] = '\0';
        new_msg->size = min_t(short, max_message_size, data_size);

        if (subscriber_message_count(&subscriber->message_queue) >= max_messages) {
            message_s *oldest_msg = list_first_entry(&subscriber->message_queue, message_s, link);
            list_del(&oldest_msg->link);
            kfree(oldest_msg);
            printk(KERN_ALERT "PubSub Driver: Discarding old message for PID %d on topic %d.\n", subscriber->pid, topic->id);
        }

        list_add_tail(&new_msg->link, &subscriber->message_queue);
    }

    printk(KERN_INFO "Message published to topic %d: \"%s\".\n", topic->id, message_data);
    return 0;
}


int topic_remove_subscriber(topic_s *topic, int pid) {
    process_s *entry, *temp;
    
    list_for_each_entry_safe(entry, temp, &topic->subscribers, subscriber_node) {
        if (entry->pid == pid) {
            if (!list_empty(&entry->message_queue)) {
                printk(KERN_ALERT "PubSub Driver: Unsubscribing PID %d with unread messages on topic %d.\n", pid, topic->id);
            }
            
            message_s *msg_entry, *msg_temp;
            list_for_each_entry_safe(msg_entry, msg_temp, &entry->message_queue, link) {
                list_del(&msg_entry->link);
                kfree(msg_entry);
            }
            
            list_del(&entry->subscriber_node);
            kfree(entry);
            printk(KERN_INFO "PID %d unsubscribed from topic %d.\n", pid, topic->id);
            return 0;
        }
    }
    
    printk(KERN_INFO "PID %d was not found as a subscriber to topic %d.\n", pid, topic->id);
    return -EINVAL;
}


message_s *subscriber_read_message(process_s *subscriber) {
    message_s *first_msg;
    if (list_empty(&subscriber->message_queue)) {
        return NULL;
    }

    first_msg = list_first_entry(&subscriber->message_queue, message_s, link);
    list_del(&first_msg->link);
    printk(KERN_INFO "Message read by PID %d: \"%s\".\n", subscriber->pid, first_msg->message);
    return first_msg;
}


void broker_cleanup(void) {
    topic_s *topic_entry, *topic_temp;
    process_s *proc_entry, *proc_temp;
    
    list_for_each_entry_safe(topic_entry, topic_temp, &my_broker.topics, topic_node) {
        list_for_each_entry_safe(proc_entry, proc_temp, &topic_entry->subscribers, subscriber_node) {
            message_s *msg_entry, *msg_temp;
            list_for_each_entry_safe(msg_entry, msg_temp, &proc_entry->message_queue, link) {
                list_del(&msg_entry->link);
                kfree(msg_entry);
            }
            list_del(&proc_entry->subscriber_node);
            kfree(proc_entry);
        }
        
        list_del(&topic_entry->topic_node);
        kfree(topic_entry);
    }
    printk(KERN_INFO "Broker cleaned up.\n");
}