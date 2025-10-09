#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/sched.h>

#include "broker.h"

/*
INICIALIZACAO E CONFIG DO DRIVER
*/

#define DEVICE_NAME "pubsub_driver"
#define CLASS_NAME  "pubsub_class"
#define MAX_COMMAND_LENGTH 256

MODULE_LICENSE("GPL");

static int majorNumber;
static int number_opens = 0;
static struct class *charClass = NULL;
static struct device *charDevice = NULL;

static int  dev_open(struct inode *, struct file *);
static int  dev_release(struct inode *, struct file *);
static ssize_t  dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t  dev_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops =
{
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .release = dev_release,
};

static int pubsub_init(void)
{
    printk(KERN_INFO "[PUBSUB] Initializing the LKM\n");

    broker_init();

    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber < 0) {
        printk(KERN_ALERT "PubSub Driver failed to register a major number\n");
        return majorNumber;
    }
    
    printk(KERN_INFO "[PUBSUB] registered correctly with major number %d\n", majorNumber);

    charClass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(charClass)) {
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "[PUBSUB] failed to register device class\n");
        return PTR_ERR(charClass);
    }
    
    printk(KERN_INFO "[PUBSUB] device class registered correctly\n");

    charDevice = device_create(charClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(charDevice)) {
        class_destroy(charClass);
        unregister_chrdev(majorNumber, DEVICE_NAME);
        printk(KERN_ALERT "[PUBSUB] failed to create the device\n");
        return PTR_ERR(charDevice);
    }
    
    printk(KERN_INFO "[PUBSUB] device class created.\n");
    return 0;
}

static void pubsub_exit(void)
{
    broker_cleanup();

    device_destroy(charClass, MKDEV(majorNumber, 0));
    class_unregister(charClass);
    class_destroy(charClass);
    unregister_chrdev(majorNumber, DEVICE_NAME);
    printk(KERN_INFO "[PUBSUB] goodbye.\n");
}

static int dev_open(struct inode *inodep, struct file *filep)
{
    number_opens++;
    printk(KERN_INFO "[PUBSUB] device has been opened %d time(s)\n", number_opens);
    printk("Process id: %d, name: %s\n", (int) task_pid_nr(current), current->comm);
    filep->private_data = NULL; // Initialize private data for state management
    return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
    int ret;
    int pid = (int)task_pid_nr(current);
    int topic_id;
    
    // Check if the file pointer has a topic associated with it (from a previous /fetch command)
    if (filep->private_data == NULL) {
        printk(KERN_INFO "[PUBSUB] No topic set for reading. Please use /fetch first.\n");
        return 0; // Return 0 to indicate no data
    }
    
    // Cast the private data back to the correct type
    topic_id = *(int *)filep->private_data;

    // Call the broker function to read a message for the given pid and topic
    // NOTE: This broker function needs to be implemented to handle per-subscriber queues
    // ret = broker_read_message_from_queue(pid, topic_id, buffer, len);

    // If there are no more messages, clean up the private data
    if (ret == 0) {
        kfree(filep->private_data);
        filep->private_data = NULL;
    }
    
    return ret;
}

static ssize_t dev_write(struct file *filep, const char __user *buffer, size_t len, loff_t *offset)
{
    char *kernel_buffer;
    char instruction[20];
    int topic_id;
    int ret = -EINVAL;
    pid_t current_pid = task_pid_nr(current);

    if (len >= MAX_COMMAND_LENGTH || len <= 1) {
        printk(KERN_INFO "[PUBSUB] Command too long or too short.\n");
        return -EINVAL;
    }

    kernel_buffer = kmalloc(len + 1, GFP_KERNEL);
    if (!kernel_buffer) {
        printk(KERN_ALERT "[PUBSUB] Failed to allocate kernel buffer.\n");
        return -ENOMEM;
    }
    if (copy_from_user(kernel_buffer, buffer, len)) {
        kfree(kernel_buffer);
        return -EFAULT;
    }
    kernel_buffer[len] = '\0';

    printk(KERN_INFO "[PUBSUB] Received command '%s'\n", kernel_buffer);

    if (sscanf(kernel_buffer, "%19s %d", instruction, &topic_id) == 2) {
        if (strcmp(instruction, "/subscribe") == 0) {
            topic_s *topic = broker_find_topic(topic_id);
            if (!topic) {
                topic = broker_create_topic(topic_id);
            }
            if (topic) {
                topic_add_subscriber(topic, current_pid);
                ret = len;
            } else {
                ret = -ENOMEM;
            }
        } else if (strcmp(instruction, "/unsubscribe") == 0) {
            topic_s *topic = broker_find_topic(topic_id);
            if (topic) {
                topic_remove_subscriber(topic, current_pid); // Pass topic without &
                ret = len;
            } else {
                printk(KERN_INFO "[PUBSUB] Topic %d not found for unsubscribing.\n", topic_id);
                ret = -EINVAL;
            }
        } else if (strcmp(instruction, "/fetch") == 0) {
            topic_s *topic = broker_find_topic(topic_id);
            if (topic) {
                // Store the topic ID in the file's private data for dev_read to use
                int *topic_ptr = kmalloc(sizeof(int), GFP_KERNEL);
                if (topic_ptr) {
                    *topic_ptr = topic_id;
                    filep->private_data = topic_ptr;
                    printk(KERN_INFO "[PUBSUB] Topic %d set for read operations.\n", topic_id);
                    ret = len;
                } else {
                    ret = -ENOMEM;
                }
            } else {
                printk(KERN_INFO "[PUBSUB] Topic %d not found for fetching.\n", topic_id);
                ret = -EINVAL;
            }
        }
    } else if (strncmp(kernel_buffer, "/publish", 8) == 0) {
        // Publish command handling for multi-word messages
        char *message_start = strchr(kernel_buffer + 8, ' ');
        if (message_start) {
            int topic_id_pub;
            char *topic_str = message_start + 1;
            sscanf(topic_str, "%d", &topic_id_pub);
            
            char *message_content = strchr(topic_str, '"');
            if (message_content) {
                message_content++; // Move past the quote
                char *end_of_message = strrchr(message_content, '"');
                if (end_of_message) {
                    *end_of_message = '\0';
                    topic_s *topic = broker_find_topic(topic_id_pub);
                    if (topic) {
                        topic_publish_message(topic, message_content, (short)strlen(message_content));
                        ret = len;
                    } else {
                        printk(KERN_INFO "[PUBSUB] Topic %d not found for publishing.\n", topic_id_pub);
                    }
                }
            }
        }
    } else {
        printk(KERN_INFO "[PUBSUB] Unknown command '%s'\n", kernel_buffer);
        ret = -EINVAL;
    }
    
    kfree(kernel_buffer);
    return (ret > 0) ? len : ret;
}


static int dev_release(struct inode *inodep, struct file *filep)
{
    // If there's any private data left, free it on close
    if (filep->private_data != NULL) {
        kfree(filep->private_data);
        filep->private_data = NULL;
    }

    printk(KERN_INFO "[PUBSUB] device successfully closed\n");
    return 0;
}

module_init(pubsub_init);
module_exit(pubsub_exit);