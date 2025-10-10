#include <linux/list.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include "broker.h"

static broker_s my_broker;

void broker_init(void) {

    INIT_LIST_HEAD(&my_broker.subscriber);
    INIT_LIST_HEAD(&my_broker.publish);

    printk(KERN_INFO "Broker initialized.\n");
}

void insert_topic_to_broker(topic_s *topic, char list_type) {
    if (!topic) {
        printk(KERN_ERR "Cannot insert a NULL topic.\n");
        return;
    }

    if (list_type == 's') {
        list_add_tail(&topic->subscribe_node, &my_broker.subscriber);
        printk(KERN_INFO "Topic '%s' added to broker's subscriber list.\n", topic->name);
    } else if (list_type == 'p') {
        list_add_tail(&topic->publish_node, &my_broker.publish);
        printk(KERN_INFO "Topic '%s' added to broker's publish list.\n", topic->name);
    } else {
        printk(KERN_WARNING "Invalid list type '%c' for topic insertion.\n", list_type);
    }
}

topic_s *broker_find_topic(char list_type, const char *name) {
    topic_s *entry;
    struct list_head *topic_list_head;

    if (list_type == 's') {
        topic_list_head = &my_broker.subscriber;
        list_for_each_entry(entry, topic_list_head, subscribe_node) {
            if (strcmp(entry->name, name) == 0) {
                printk(KERN_INFO "Found topic '%s' in the subscriber list.\n", name);
                return entry;
            }
        }
    } else if (list_type == 'p') {
        topic_list_head = &my_broker.publish;
        list_for_each_entry(entry, topic_list_head, publish_node) {
            if (strcmp(entry->name, name) == 0) {
                printk(KERN_INFO "Found topic '%s' in the publish list.\n", name);
                return entry;
            }
        }
    } else {
        printk(KERN_WARNING "Invalid list type '%c' for topic search.\n", list_type);
    }

    printk(KERN_INFO "Topic '%s' not found in the specified list.\n", name);
    return NULL;
}

//deve se criar para sub tb????????
topic_s *broker_find_or_create_topic(char list_type, const char *name) {
    topic_s *topic;
    
    topic = broker_find_topic(list_type, name);

    if (!topic && list_type == 'p') {
        topic = kmalloc(sizeof(*topic), GFP_KERNEL);
        if (!topic) {
            printk(KERN_ERR "Failed to create new topic '%s'.\n", name);
            return NULL;
        }

        topic->name = kstrdup(name, GFP_KERNEL);
        if (!topic->name) {
            kfree(topic);
            return NULL;
        }

        INIT_LIST_HEAD(&topic->message_queue);
        INIT_LIST_HEAD(&topic->publish_node);
        INIT_LIST_HEAD(&topic->subscribe_node);

        insert_topic_to_broker(topic, 'p');
        printk(KERN_INFO "New topic '%s' created and added to publish list.\n", name);
    } else if (!topic && list_type == 's') {
        printk(KERN_ERR "Cannot subscribe to a non-existent topic '%s'.\n", name);
        return NULL;
    }
    
    return topic;
}

int register_process_to_topic(const char *topic_name, char list_type, int pid) {
    topic_s *topic;
    process_s *new_process;
    
    topic = broker_find_or_create_topic(list_type, topic_name);
    if (!topic) {
        return -EINVAL;
    }
    
    new_process = kmalloc(sizeof(*new_process), GFP_KERNEL);
    if (!new_process) {
        printk(KERN_ERR "Failed to allocate memory for new process node.\n");
        return -ENOMEM;
    }

    new_process->pid = pid;

    if (list_type == 's') {
        INIT_LIST_HEAD(&new_process->subscriber_node);
        list_add_tail(&new_process->subscriber_node, &topic->subscribe_node);
        printk(KERN_INFO "Process PID %d added as a subscriber to topic '%s'.\n", pid, topic->name);
    } else if (list_type == 'p') {
        INIT_LIST_HEAD(&new_process->publish_node);
        list_add_tail(&new_process->publish_node, &topic->publish_node);
        printk(KERN_INFO "Process PID %d added as a publisher to topic '%s'.\n", pid, topic->name);
    } else {
        printk(KERN_WARNING "Invalid list type '%c' for adding process PID %d to topic '%s'.\n", list_type, pid, topic->name);
        kfree(new_process);
        return -EINVAL;
    }
    
    return 0;
}


//alterar para o nome ou strutc de topic???????????
int topic_publish_message(topic_s *topic, const char *message_data, short max_size) {
    message_s *new_message;
    size_t data_size = strnlen(message_data, max_size) + 1;

    if (!topic) {
        printk(KERN_ERR "Cannot publish message to a NULL topic.\n");
        return -EINVAL;
    }

    new_message = kmalloc(sizeof(*new_message), GFP_KERNEL);
    if (!new_message) {
        printk(KERN_ERR "Failed to allocate memory for new message.\n");
        return -ENOMEM;
    }

    new_message->message = kmalloc(data_size, GFP_KERNEL);
    if (!new_message->message) {
        printk(KERN_ERR "Failed to allocate memory for message data.\n");
        kfree(new_message);
        return -ENOMEM;
    }
    strncpy(new_message->message, message_data, data_size);
    new_message->size = data_size;

    INIT_LIST_HEAD(&new_message->link);
    list_add_tail(&new_message->link, &topic->message_queue);

    printk(KERN_INFO "Message published to topic '%s'.\n", topic->name);

    return 0;
}
