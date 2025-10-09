#include <linux/slab.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include "list_driver.h"

int list_add_entry(struct list_head *head, const char *data)
{
    struct message_s *new_node = kmalloc(sizeof(struct message_s), GFP_KERNEL);
    
    if (!new_node) {
        printk(KERN_ERR "Memory allocation failed.\n");
        return 1;
    }
    
    strncpy(new_node->message, data, MSG_SIZE);
    new_node->size = strnlen(data, MSG_SIZE);
    
    list_add_tail(&(new_node->link), head);
    
    return 0;
}

void list_show(struct list_head *head)
{
    struct message_s *entry = NULL;
    int i = 0;
    
    list_for_each_entry(entry, head, link) {
        printk(KERN_INFO "Message #%d: %s\n", i++, entry->message);
    }
}

int list_delete_head(struct list_head *head)
{
    struct message_s *entry = NULL;
    
    if (list_empty(head)) {
        printk(KERN_INFO "Empty list.\n");
        return 1;
    }
    
    entry = list_first_entry(head, struct message_s, link);
    
    list_del(&entry->link);
    kfree(entry);
        
    return 0;
}

int list_delete_entry(struct list_head *head, const char *data)
{
    struct message_s *entry = NULL;
    
    list_for_each_entry(entry, head, link) {
        if (strcmp(entry->message, data) == 0) {
            list_del(&(entry->link));
            kfree(entry);
            return 0;
        }
    }
    
    printk(KERN_INFO "Could not find data.");
    return 1;
}