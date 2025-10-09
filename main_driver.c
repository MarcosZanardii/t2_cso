#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/slab.h>
#include "broker.h"

/*
INICIALIZACAO E CONFIG DO DRIVER
*/

#define DEVICE_NAME "pubsub_driver"
#define CLASS_NAME  "pubsub_class"

MODULE_LICENSE("GPL");

static int majorNumber;
static int number_opens = 0;
static struct class *charClass = NULL;
static struct device *charDevice = NULL;

struct list_head list;

static int	dev_open(struct inode *, struct file *);
static int	dev_release(struct inode *, struct file *);
static ssize_t	dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t	dev_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops =
{
	.open = dev_open,
	.read = dev_read,
	.write = dev_write,
	.release = dev_release,
};

static int pubsub_init(void)
{
	printk(KERN_INFO "PubSub Driver: Initializing the LKM\n");

	// Try to dynamically allocate a major number for the device -- more difficult but worth it
	majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
	if (majorNumber < 0) {
		printk(KERN_ALERT "PubSub Driver failed to register a major number\n");
		return majorNumber;
	}
	
	printk(KERN_INFO "PubSub Driver: registered correctly with major number %d\n", majorNumber);

	// Register the device class
	charClass = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(charClass)) {		// Check for error and clean up if there is
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT "PubSub Driver: failed to register device class\n");
		return PTR_ERR(charClass);	// Correct way to return an error on a pointer
	}
	
	printk(KERN_INFO "PubSub Driver: device class registered correctly\n");

	// Register the device driver
	charDevice = device_create(charClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
	if (IS_ERR(charDevice)) {		// Clean up if there is an error
		class_destroy(charClass);
		unregister_chrdev(majorNumber, DEVICE_NAME);
		printk(KERN_ALERT "PubSub Driver: failed to create the device\n");
		return PTR_ERR(charDevice);
	}
	
	printk(KERN_INFO "PubSub Driver: device class created.\n");
	
	INIT_LIST_HEAD(&list);
		
	return 0;
}

static void pubsub_exit(void)
{
	device_destroy(charClass, MKDEV(majorNumber, 0));
	class_unregister(charClass);
	class_destroy(charClass);
	unregister_chrdev(majorNumber, DEVICE_NAME);
	printk(KERN_INFO "PubSub Driver: goodbye.\n");
}

static int dev_open(struct inode *inodep, struct file *filep)
{
	number_opens++;
	printk(KERN_INFO "PubSub Driver: device has been opened %d time(s)\n", number_opens);
	printk("Process id: %d, name: %s\n", (int) task_pid_nr(current), current->comm);

	return 0;
}

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
	int error = 0;
	struct message_s *entry = list_first_entry(&list, struct message_s, link);
   
	if (list_empty(&list)) {
		printk(KERN_INFO "PubSub Driver: no data.\n");
		
		return 0;
	}	
	
	// copy_to_user has the format ( * to, *from, size) and returns 0 on success
	error = copy_to_user(buffer, entry->message, entry->size);

	if (!error) {				// if true then have success
		printk(KERN_INFO "PubSub Driver: sent %d characters to the user\n", entry->size);
		list_delete_head();
		
		return 0;
	} else {
		printk(KERN_INFO "PubSub Driver: failed to send %d characters to the user\n", error);
		
		return -EFAULT;			// Failed -- return a bad address message (i.e. -14)
	}
}

static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset)
{
	if (len < MSG_SIZE) {
		list_add_entry(buffer);
		list_show();

		printk(KERN_INFO "PubSub Driver: received %zu characters from the user\n", len);
		
		return len;
	} else {
		printk(KERN_INFO "PubSub Driver: too many characters to deal with (%d)\n", len);
		
		return 0;
	}
}

static int dev_release(struct inode *inodep, struct file *filep)
{
	printk(KERN_INFO "PubSub Driver: device successfully closed\n");

	return 0;
}

module_init(pubsub_init);
module_exit(pubsub_exit);
