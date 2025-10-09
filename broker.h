#include "list_driver.h"

typedef struct {
    int pid;
    struct list_head subscriber_node;//elo que relaciona nodo no sub
    struct list_head publisher_node;//elo que relaciona nodo no pub
} process_s;

typedef struct {
    int id;
    struct list_head topic_node;//lista de topicos
    struct list_head subscribers;//subs relacionado a determinado topico
    struct list_head publishers;//subs relacionado a determinado topico
    struct list_head messages;//lista de mensagens naquele topico, DEVE TER MAX POR PARAM      
} topic_s;

typedef struct {
    struct list_head topics;//lista de topicos gerenciado pelo broker
} broker_s;