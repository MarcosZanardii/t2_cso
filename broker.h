#ifndef BROKER_H
#define BROKER_H

#include <linux/list.h>
#include <linux/slab.h>

extern int max_msg_size; 
extern int max_msg_n;

typedef struct {
    char *message;
    size_t size;
    struct list_head link;
} message_s;

typedef struct {
    int pid;
    int msg_count;                  
    struct list_head message_queue;   
    struct list_head publish_node;    
    struct list_head subscriber_node; 
} process_s;


typedef struct topic {
    char *name;
    int msg_count;
    
    struct list_head publish_node;        
    struct list_head subscribe_node;      
    
    struct list_head process_subscribers; 
    struct list_head process_publishers;  
} topic_s;

typedef struct {
    struct list_head subscriber; 
    struct list_head publish;   
    int max_msg;
} broker_s;

void broker_init(void);
topic_s *create_topic(const char *name);
process_s *create_process(int pid);
topic_s *find_topic(const char *name);
process_s *find_process(int pid);
void insert_topic_to_broker(topic_s *topic, char list_type);
int is_pid_in_subscribers(int pid, topic_s *topic);
int is_pid_in_publishers(int pid, topic_s *topic);
int register_process_to_topic(const char *topic_name, char list_type, int pid);
int topic_publish_message(topic_s *topic, const char *message_data, short max_size);
void topic_remove_subscriber(topic_s *topic, int pid);
void show_topics(void);

#endif