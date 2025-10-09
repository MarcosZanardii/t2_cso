#ifndef BROKER_H
#define BROKER_H

#include <linux/list.h>
#include <linux/slab.h>

// Define a structure for messages
typedef struct {
    char message[250];// Assuming max message size is 250 bytes [cite: 19]
    size_t size;
    struct list_head link;
} message_s;

typedef struct {
    int pid;
    struct list_head subscriber_node;
    struct list_head message_queue; // Individual message queue for each subscriber
} process_s;

typedef struct {
    int id;
    struct list_head topic_node;
    struct list_head subscribers;
} topic_s;

typedef struct {
    struct list_head topics;
} broker_s;

// Function prototypes
void broker_init(void);
topic_s *broker_create_topic(int id);
topic_s *broker_find_topic(int id);
process_s *topic_find_subscriber(topic_s *topic, int pid);
int topic_add_subscriber(topic_s *topic, int pid);
int topic_publish_message(topic_s *topic, const char *message_data, short max_size);
int topic_remove_subscriber(topic_s *topic, int pid);
message_s *subscriber_read_message(process_s *subscriber);
void broker_cleanup(void);

#endif