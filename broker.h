#ifndef BROKER_H
#define BROKER_H

#include <linux/list.h>
#include <linux/slab.h>

extern int max_msg_size; 
extern int max_msg_n;

// Define a structure for messages
typedef struct {
    char *message;
    size_t size;
    struct list_head link;
} message_s;

typedef struct {
    int pid;
    struct list_head process_node;
    struct list_head subscriber_node;
    struct list_head publish_node;
} process_s;

typedef struct {
    char *name;
    struct list_head message_queue; //lista de mensagens
    struct list_head publish_node;//nodo na lista de publish do broker
    struct list_head subscribe_node;//nodo na lista de subscriber do broker
    struct list_head process;//lista de processos nesse topico
} topic_s;

typedef struct {
    struct list_head subscriber;
    struct list_head publish;
    int max_msg;
} broker_s;

void broker_init(void);
void insert_topic_to_broker(topic_s *topic, char list_type);
topic_s *broker_find_topic(char list_type, const char *name);
topic_s *broker_find_or_create_topic(char list_type, const char *name);
int register_process_to_topic(const char *topic_name, char list_type, int pid);
int topic_publish_message(topic_s *topic, const char *message_data, short max_size);

#endif