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

topic_s *find_topic(const char *name) {
    topic_s *entry;
    
    list_for_each_entry(entry, &my_broker.publish, publish_node) {
        if (strcmp(entry->name, name) == 0) {
            printk(KERN_INFO "Found topic '%s' in the publish list.\n", name);
            return entry;
        }
    }

    list_for_each_entry(entry, &my_broker.subscriber, subscribe_node) {
        if (strcmp(entry->name, name) == 0) {
            printk(KERN_INFO "Found topic '%s' in the subscriber list.\n", name);
            return entry;
        }
    }
    printk(KERN_INFO "Topic '%s' not found in either list.\n", name);
    return NULL;
}

topic_s *create_topic(const char *name) {
    topic_s *topic;
    
    topic = kmalloc(sizeof(*topic), GFP_KERNEL);
    if (!topic) {
        printk(KERN_ERR "Failed to allocate memory for new topic.\n");
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
    
    printk(KERN_INFO "New topic '%s' created and added to both lists.\n", name);
    return topic;
}

process_s *create_process(int pid) {
    process_s *process;
    
    process = kmalloc(sizeof(*process), GFP_KERNEL);
    if (!process) {
        printk(KERN_ERR "Failed to allocate memory for new process.\n");
        return NULL;
    }

    process->pid = pid;
    if (!process->pid) {
        kfree(process);
        return NULL;
    }

    INIT_LIST_HEAD(&process->process_node);
    INIT_LIST_HEAD(&process->publish_node);
    INIT_LIST_HEAD(&process->subscriber_node);
    
    printk(KERN_INFO "New process '%d' created and added to both lists.\n", pid);
    return process;
}

int register_process_to_topic(const char *topic_name, char list_type, int pid) {
    topic_s *topic;
    process_s *new_process;
    
    topic = find_topic(topic_name);

    if (!topic) {
        topic = create_topic(topic_name);
    }
    
    if (!topic) {
        printk(KERN_ERR "Failed to find or create topic '%s'.\n", topic_name);
        return -ENOMEM;
    }

    new_process = create_process(pid);

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

void topic_remove_subscriber(topic_s *topic, int pid) {
    process_s *process, *temp;
    
    if (!topic) {
        printk(KERN_ERR "Cannot remove subscriber from a NULL topic.\n");
        return;
    }

    list_for_each_entry_safe(process, temp, &topic->subscribe_node, subscriber_node) {
        if (process->pid == pid) {
            printk(KERN_INFO "Removing subscriber with PID %d from topic '%s'.\n", pid, topic->name);
            list_del(&process->subscriber_node);
            kfree(process);
            return;
        }
    }
    
    printk(KERN_INFO "Subscriber with PID %d not found in topic '%s'.\n", pid, topic->name);
}