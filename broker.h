#ifndef BROKER_H
#define BROKER_H

#include <linux/list.h>
#include <linux/slab.h>

extern int max_msg_size; 
extern int max_msg_n;

typedef struct {
    char *message;
    size_t size;
    struct list_head link;          // Nó para a lista 'message_queue' de um tópico
} message_s;

typedef struct {
    int pid;
    // 'process_node' removido pois não era utilizado.
    struct list_head subscriber_node; // Nó para a lista 'process_subscribers' de um tópico
    struct list_head publish_node;    // Nó para a lista 'process_publishers' de um tópico
} process_s;

typedef struct topic {
    char *name;
    struct list_head message_queue;       // Cabeça da lista de mensagens (message_s)
    
    // Nós para conectar este tópico às listas principais do broker
    struct list_head publish_node;        // Nó para a lista 'publish' do broker
    struct list_head subscribe_node;      // Nó para a lista 'subscriber' do broker

    // CORREÇÃO: Cabeças de lista para os processos associados a este tópico
    struct list_head process_subscribers; // Cabeça da lista de processos inscritos (process_s)
    struct list_head process_publishers;  // Cabeça da lista de processos publicadores (process_s)
} topic_s;

typedef struct {
    struct list_head subscriber; // Cabeça da lista de tópicos com inscritos (topic_s)
    struct list_head publish;    // Cabeça da lista de tópicos com publicadores (topic_s)
    int max_msg;
} broker_s;

void broker_init(void);
topic_s *create_topic(const char *name);
process_s *create_process(int pid);
topic_s *find_topic(const char *name);
void insert_topic_to_broker(topic_s *topic, char list_type);
int register_process_to_topic(const char *topic_name, char list_type, int pid);
int topic_publish_message(topic_s *topic, const char *message_data, short max_size);
void topic_remove_subscriber(topic_s *topic, int pid);
void show_topics(void);

#endif