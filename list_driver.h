#include <linux/list.h>

#define MSG_SIZE    128//deve ser por parametro

typedef struct{
    struct list_head link;
    char message[MSG_SIZE];
    short size;
} message_s ;

int list_add_entry(struct list_head *head, const char *data);
void list_show(struct list_head *head);
int list_delete_head(struct list_head *head);
int list_delete_entry(struct list_head *head, const char *data);	