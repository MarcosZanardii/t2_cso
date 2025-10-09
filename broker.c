#include <linux/list.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include "broker.h"

// A instância global do broker para o nosso módulo
static broker_s my_broker;

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
    
    //inicializa listas relacionada a esse topico
    INIT_LIST_HEAD(&new_topic->subscribers);
    INIT_LIST_HEAD(&new_topic->publishers);
    INIT_LIST_HEAD(&new_topic->messages);

    //adiciona o novo topico a lista de topicos do broker
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

int topic_add_subscriber(topic_s *topic, int pid) {

    process_s *new_process = kmalloc(sizeof(*new_process), GFP_KERNEL);

    if (!new_process) {
        printk(KERN_ERR "Failed to allocate memory for subscriber.\n");
        return -ENOMEM;
    }

    new_process->pid = pid;

    //relaciona o sub do processo a lista de subs do broker
    list_add_tail(&new_process->subscriber_node, &topic->subscribers);

    printk(KERN_INFO "PID %d added as subscriber to topic %d.\n", pid, topic->id);
    return 0;
}

int topic_publish_message(topic_s *topic, const char *message_data, short max_size) {
    //adicionar o pid de quem publicou**********
    message_s *new_msg = kmalloc(sizeof(*new_msg), GFP_KERNEL);
    if (!new_msg) {
        printk(KERN_ERR "Failed to allocate memory for new message.\n");
        return -ENOMEM;
    }

    strncpy(new_msg->message, message_data, min_t(short, max_size, MSG_SIZE));
    new_msg->message[min_t(short, max_size, MSG_SIZE) - 1] = '\0'; // Garantir terminação de string
    new_msg->size = min_t(short, max_size, MSG_SIZE);

    //adiciona mensagem no topico
    list_add_tail(&new_msg->link, &topic->messages);
    printk(KERN_INFO "Message published to topic %d: \"%s\".\n", topic->id, new_msg->message);
    return 0;
}


message_s *topic_read_message(topic_s *topic) {
    message_s *first_msg;
    if (list_empty(&topic->messages)) {
        return NULL;
    }

    first_msg = list_first_entry(&topic->messages, message_s, link);
    list_del(&first_msg->link);
    printk(KERN_INFO "Message read from topic %d: \"%s\".\n", topic->id, first_msg->message);

    return first_msg;
}

void broker_cleanup(void) {
    topic_s *topic_entry, *topic_temp;
    message_s *msg_entry, *msg_temp;
    process_s *proc_entry, *proc_temp;
    
    // 1. Libera todos os tópicos
    list_for_each_entry_safe(topic_entry, topic_temp, &my_broker.topics, topic_node) {
        // 2. Para cada tópico, libera as mensagens e os processos
        list_for_each_entry_safe(msg_entry, msg_temp, &topic_entry->messages, link) {
            list_del(&msg_entry->link);
            kfree(msg_entry);
        }
        list_for_each_entry_safe(proc_entry, proc_temp, &topic_entry->subscribers, subscriber_node) {
            list_del(&proc_entry->subscriber_node);
            kfree(proc_entry);
        }
        list_for_each_entry_safe(proc_entry, proc_temp, &topic_entry->publishers, publisher_node) {
            list_del(&proc_entry->publisher_node);
            kfree(proc_entry);
        }
        list_del(&topic_entry->topic_node);
        kfree(topic_entry);
    }
    printk(KERN_INFO "Broker cleaned up.\n");
}