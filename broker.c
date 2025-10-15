#include <linux/list.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include "broker.h"

static broker_s my_broker;

void broker_init(void)
{
    INIT_LIST_HEAD(&my_broker.subscriber);
    INIT_LIST_HEAD(&my_broker.publish);
    printk(KERN_INFO "[BROKER_INIT] Broker initialized.\n");
}

void insert_topic_to_broker(topic_s *topic, char list_type)
{
    if (!topic) {
        return;
    }
    if (list_type == 's') {
        list_add_tail(&topic->subscribe_node, &my_broker.subscriber);
        printk(KERN_INFO "[INSERT_TOPIC] Topic '%s' added to subscriber list.\n", topic->name);
    } else if (list_type == 'p') {
        list_add_tail(&topic->publish_node, &my_broker.publish);
        printk(KERN_INFO "[INSERT_TOPIC] Topic '%s' added to publish list.\n", topic->name);
    } else {
        printk(KERN_WARNING "[INSERT_TOPIC] Invalid list type '%c'.\n", list_type);
    }
}

int is_pid_in_subscribers(int pid, topic_s *topic)
{
    process_s *proc_entry;
    list_for_each_entry(proc_entry, &topic->process_subscribers, subscriber_node) {
        if (proc_entry->pid == pid) {
            return 1;
        }
    }
    return 0;
}

int is_pid_in_publishers(int pid, topic_s *topic)
{
    process_s *proc_entry;
    list_for_each_entry(proc_entry, &topic->process_publishers, publish_node) {
        if (proc_entry->pid == pid) {
            return 1;
        }
    }
    return 0;
}

topic_s *find_topic(const char *name)
{
    topic_s *entry;

    printk(KERN_INFO "[FIND_TOPIC] Searching for topic: %s.\n", name);

    list_for_each_entry(entry, &my_broker.publish, publish_node) {
        if (strcmp(entry->name, name) == 0) {
            printk(KERN_INFO "[FIND_TOPIC] Found topic '%s' in publish list.\n", name);
            return entry;
        }
    }

    list_for_each_entry(entry, &my_broker.subscriber, subscribe_node) {
        if (strcmp(entry->name, name) == 0) {
            printk(KERN_INFO "[FIND_TOPIC] Found topic '%s' in subscriber list.\n", name);
            return entry;
        }
    }
    
    printk(KERN_INFO "[FIND_TOPIC] Topic '%s' not found.\n", name);
    return NULL;
}

topic_s *create_topic(const char *name)
{
    topic_s *topic;

    printk(KERN_INFO "[CREATE_TOPIC] Creating topic: %s.\n", name);

    topic = kmalloc(sizeof(*topic), GFP_KERNEL);
    if (!topic) {
        printk(KERN_ERR "[CREATE_TOPIC] Failed to allocate memory for topic '%s'.\n", name);
        return NULL;
    }

    topic->name = kstrdup(name, GFP_KERNEL);
    if (!topic->name) {
        printk(KERN_ERR "[CREATE_TOPIC] Failed to allocate memory for topic name.\n");
        kfree(topic);
        return NULL;
    }

    topic->msg_count = 0;
    INIT_LIST_HEAD(&topic->message_queue);
    INIT_LIST_HEAD(&topic->publish_node);
    INIT_LIST_HEAD(&topic->subscribe_node);
    INIT_LIST_HEAD(&topic->process_subscribers);
    INIT_LIST_HEAD(&topic->process_publishers);

    printk(KERN_INFO "[CREATE_TOPIC] Topic '%s' created.\n", topic->name);
    return topic;
}

process_s *create_process(int pid)
{
    process_s *process;

    printk(KERN_INFO "[CREATE_PROCESS] Creating process for PID: %d.\n", pid);

    process = kmalloc(sizeof(*process), GFP_KERNEL);
    if (!process) {
        printk(KERN_ERR "[CREATE_PROCESS] Failed to allocate memory for new process.\n");
        return NULL;
    }

    process->pid = pid;
    INIT_LIST_HEAD(&process->publish_node);
    INIT_LIST_HEAD(&process->subscriber_node);

    printk(KERN_INFO "[CREATE_PROCESS] Process for PID '%d' created.\n", pid);
    return process;
}

process_s *find_process(int pid)
{
    topic_s *topic_entry;
    process_s *process_entry;

    printk(KERN_INFO "[FIND_PROCESS] Searching for process with PID: %d.\n", pid);

    list_for_each_entry(topic_entry, &my_broker.publish, publish_node) {
        list_for_each_entry(process_entry, &topic_entry->process_publishers, publish_node) {
            if (process_entry->pid == pid) {
                printk(KERN_INFO "[FIND_PROCESS] Found PID %d as publisher in topic '%s'.\n", pid, topic_entry->name);
                return process_entry;
            }
        }
        list_for_each_entry(process_entry, &topic_entry->process_subscribers, subscriber_node) {
            if (process_entry->pid == pid) {
                printk(KERN_INFO "[FIND_PROCESS] Found PID %d as subscriber in topic '%s'.\n", pid, topic_entry->name);
                return process_entry;
            }
        }
    }

    list_for_each_entry(topic_entry, &my_broker.subscriber, subscribe_node) {
        list_for_each_entry(process_entry, &topic_entry->process_subscribers, subscriber_node) {
            if (process_entry->pid == pid) {
                printk(KERN_INFO "[FIND_PROCESS] Found PID %d as subscriber in topic '%s'.\n", pid, topic_entry->name);
                return process_entry;
            }
        }
        list_for_each_entry(process_entry, &topic_entry->process_publishers, publish_node) {
            if (process_entry->pid == pid) {
                printk(KERN_INFO "[FIND_PROCESS] Found PID %d as publisher in topic '%s'.\n", pid, topic_entry->name);
                return process_entry;
            }
        }
    }

    printk(KERN_WARNING "[FIND_PROCESS] Process with PID %d not found in any topic.\n", pid);
    return NULL;
}

int register_process_to_topic(const char *topic_name, char list_type, int pid)
{
    topic_s *topic = find_topic(topic_name);
    process_s *new_process;

    if (!topic) {
        topic = create_topic(topic_name);
        if (!topic) {
            return -ENOMEM;
        }
        insert_topic_to_broker(topic, list_type);
    } else {
        if (list_type == 'p' && list_empty(&topic->publish_node)) {
            printk(KERN_INFO "[REGISTER] Linking existing topic '%s' to publish list.\n", topic_name);
            list_add_tail(&topic->publish_node, &my_broker.publish);
        } else if (list_type == 's' && list_empty(&topic->subscribe_node)) {
            printk(KERN_INFO "[REGISTER] Linking existing topic '%s' to subscriber list.\n", topic_name);
            list_add_tail(&topic->subscribe_node, &my_broker.subscriber);
        }
    }

    if (list_type == 's') {
        if (is_pid_in_subscribers(pid, topic)) {
            printk(KERN_WARNING "[REGISTER] PID %d is already a subscriber of topic '%s'.\n", pid, topic->name);
            return 0;
        }
    } else if (list_type == 'p') {
        if (is_pid_in_publishers(pid, topic)) {
            printk(KERN_WARNING "[REGISTER] PID %d is already a publisher of topic '%s'.\n", pid, topic->name);
            return 0;
        }
    }

    new_process = create_process(pid);
    if (!new_process) {
        return -ENOMEM;
    }

    if (list_type == 's') {
        list_add_tail(&new_process->subscriber_node, &topic->process_subscribers);
        printk(KERN_INFO "[REGISTER] PID %d added as subscriber to topic '%s'.\n", pid, topic->name);
    } else if (list_type == 'p') {
        list_add_tail(&new_process->publish_node, &topic->process_publishers);
        printk(KERN_INFO "[REGISTER] PID %d added as publisher to topic '%s'.\n", pid, topic->name);
    } else {
        printk(KERN_WARNING "[REGISTER] Invalid list type '%c' for PID %d.\n", list_type, pid);
        kfree(new_process);
        return -EINVAL;
    }

    return 0;
}

int topic_publish_message(topic_s *topic, const char *message_data, short max_size)
{
    message_s *new_message;
    message_s *oldest_message;
    size_t data_size;

    if (!topic) {
        printk(KERN_ERR "[PUBLISH] Cannot publish to a NULL topic.\n");
        return -EINVAL;
    }

    if (max_msg_n > 0 && topic->msg_count >= max_msg_n) {
        if (!list_empty(&topic->message_queue)) {
            oldest_message = list_first_entry(&topic->message_queue, message_s, link);
            printk(KERN_INFO "[PUBLISH] Topic '%s' reached limit (%d). Discarding oldest message.\n", topic->name, max_msg_n);
            list_del(&oldest_message->link);
            kfree(oldest_message->message);
            kfree(oldest_message);
            topic->msg_count--;
        }
    }

    data_size = strnlen(message_data, max_size) + 1;
    new_message = kmalloc(sizeof(*new_message), GFP_KERNEL);
    if (!new_message) {
        return -ENOMEM;
    }

    new_message->message = kmalloc(data_size, GFP_KERNEL);
    if (!new_message->message) {
        kfree(new_message);
        return -ENOMEM;
    }
    strncpy(new_message->message, message_data, data_size);
    new_message->size = data_size;

    INIT_LIST_HEAD(&new_message->link);
    list_add_tail(&new_message->link, &topic->message_queue);
    topic->msg_count++;

    printk(KERN_INFO "[PUBLISH] Message published to topic '%s'. Total: %d\n", topic->name, topic->msg_count);

    return 0;
}

void topic_remove_subscriber(topic_s *topic, int pid)
{
    process_s *process, *temp;

    if (!topic) {
        printk(KERN_ERR "[REMOVE_SUB] Cannot remove from a NULL topic.\n");
        return;
    }

    list_for_each_entry_safe(process, temp, &topic->process_subscribers, subscriber_node) {
        if (process->pid == pid) {
            printk(KERN_INFO "[REMOVE_SUB] Removing subscriber PID %d from topic '%s'.\n", pid, topic->name);
            list_del(&process->subscriber_node);
            kfree(process);
            return;
        }
    }

    printk(KERN_WARNING "[REMOVE_SUB] Subscriber PID %d not found in topic '%s'.\n", pid, topic->name);
}

static void print_topic_details(topic_s *topic)
{
    process_s *proc_entry;
    message_s *msg_entry;

    // Informação principal do tópico
    printk(KERN_INFO "-> Topic: \"%s\" (Messages: %d)\n", topic->name, topic->msg_count);

    // Lista de Publicadores (Publishers)
    printk(KERN_INFO "   - Publishers:");
    if (list_empty(&topic->process_publishers)) {
        printk(KERN_CONT " [None]\n");
    } else {
        printk(KERN_CONT "\n");
        list_for_each_entry(proc_entry, &topic->process_publishers, publish_node) {
            printk(KERN_INFO "     - PID: %d\n", proc_entry->pid);
        }
    }

    // Lista de Inscritos (Subscribers)
    printk(KERN_INFO "   - Subscribers:");
    if (list_empty(&topic->process_subscribers)) {
        printk(KERN_CONT " [None]\n");
    } else {
        printk(KERN_CONT "\n");
        list_for_each_entry(proc_entry, &topic->process_subscribers, subscriber_node) {
            printk(KERN_INFO "     - PID: %d\n", proc_entry->pid);
        }
    }

    // Fila de Mensagens
    printk(KERN_INFO "   - Message Queue:");
    if (list_empty(&topic->message_queue)) {
        printk(KERN_CONT " [Empty]\n");
    } else {
        printk(KERN_CONT "\n");
        list_for_each_entry(msg_entry, &topic->message_queue, link) {
            printk(KERN_INFO "     - \"%s\"\n", msg_entry->message);
        }
    }
}

void show_topics(void)
{
    topic_s *entry;

    printk(KERN_INFO "\n=============== BROKER STATE ===============\n");

    printk(KERN_INFO "--- Topics with Subscribers ---\n");
    if (list_empty(&my_broker.subscriber)) {
        printk(KERN_INFO "No topics found in subscriber list.\n");
    } else {
        list_for_each_entry(entry, &my_broker.subscriber, subscribe_node) {
            print_topic_details(entry);
        }
    }

    printk(KERN_INFO "\n--- Topics with Publishers ---\n");
    if (list_empty(&my_broker.publish)) {
        printk(KERN_INFO "No topics found in publish list.\n");
    } else {
        list_for_each_entry(entry, &my_broker.publish, publish_node) {
            print_topic_details(entry);
        }
    }
    printk(KERN_INFO "==========================================\n");
}