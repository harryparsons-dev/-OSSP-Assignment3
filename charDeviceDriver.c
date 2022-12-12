/*
 *  chardev.c: Creates a read-only char device that says how many times
 *  you've read from the dev file
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h> /* for put_user */
#include <charDeviceDriver.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");

struct k_list
{
	struct list_head list;
	char *text;
};

// static LIST_HEAD(Head_node);
static struct list_head *head;
static struct k_list *node;
char *msgIn;
/*
 * This function is called whenever a process tries to do an ioctl on our
 * device file. We get two extra parameters (additional to the inode and file
 * structures, which all device functions get): the number of the ioctl called
 * and the parameter given to the ioctl function.
 *
 * If the ioctl is write or read/write (meaning output is returned to the
 * calling process), the ioctl call returns the output of this function.
 *
 */

DEFINE_MUTEX(devLock);
// static int counter = 0;

/*
 * This function is called when the module is loaded
 */

int init_module(void)
{
	Major = register_chrdev(0, DEVICE_NAME, &fops);

	if (Major < 0)
	{
		printk(KERN_ALERT "Registering char device failed with %d\n", Major);
		return Major;
	}

	printk(KERN_INFO "I was assigned major number %d. To talk to\n", Major);
	printk(KERN_INFO "the driver, create a dev file with\n");
	printk(KERN_INFO "'mknod /dev/%s c %d 0'.\n", DEVICE_NAME, Major);
	printk(KERN_INFO "Try various minor numbers. Try to cat and echo to\n");
	printk(KERN_INFO "the device file.\n");
	printk(KERN_INFO "Remove the device file and module when done.\n");

	head = kmalloc(sizeof(struct list_head *), GFP_KERNEL);
	INIT_LIST_HEAD(head);
	return SUCCESS;
}

/*
 * This function is called when the module is unloaded
 */
void cleanup_module(void)
{
	/*  Unregister the device */
	kfree(head);
	kfree(node);
	kfree(msgIn);
	unregister_chrdev(Major, DEVICE_NAME);
}

/*
 * Methods
 */

/*
 * Called when a process tries to open the device file, like
 * "cat /dev/mycharfile"
 */

static int device_open(struct inode *inode, struct file *file)
{
	// node *text = getFirst(head);
	mutex_lock(&devLock);
	if (Device_Open)
	{
		mutex_unlock(&devLock);
		return -EBUSY;
	}
	Device_Open++;
	mutex_unlock(&devLock);
	// sprintf(msg, "I already told you %d times Hello world!\n", counter++);
	// sprintf(msg, (text->text));
	try_module_get(THIS_MODULE);

	return SUCCESS;
}

/* Called when a process closes the device file. */
static int device_release(struct inode *inode, struct file *file)
{
	mutex_lock(&devLock);
	Device_Open--; /* We're now ready for our next caller */
	mutex_unlock(&devLock);
	/*
	 * Decrement the usage count, or else once you opened the file, you'll
	 * never get get rid of the module.
	 */
	module_put(THIS_MODULE);

	return 0;
}

/*
 * Called when a process, which already opened the dev file, attempts to
 * read from it.
 */
static ssize_t device_read(struct file *filp, /* see include/linux/fs.h   */
						   char *buffer,	  /* buffer to fill with data */
						   size_t length,	  /* length of the buffer     */
						   loff_t *offset)
{
	/* result of function calls */
	// int result;

	// int count = 0;
	// struct k_list *temp;
	struct k_list *node;
	char *msgOut;
	// struct k_list *node;

	//  char *msg2 = MSG->text;

	// strcpy(msg2, MSG->text);
	/*
	 * Actually put the data into the buffer
	 */

	// if (strlen(msg) + 1 < length)
	// {
	// 	length = strlen(msg) + 1;
	// }
	// node *q = list_first_entry(&msgStore, node, list);
	//  printk(KERN_INFO "From the list {%s}", q->text);
	mutex_lock(&devLock);
	// list_for_each_entry(temp, head, list)
	// {
	// 	printk(KERN_INFO "Node %d data = %s\n", count++, temp->text);
	// }
	// printk(KERN_INFO "Total Nodes = %d\n", count);
	if (list_empty(head))
	{
		mutex_unlock(&devLock);
		return -EAGAIN;
	}
	else
	{

		node = list_first_entry(head, struct k_list, list);
		// char *msgOut = (char *)kmalloc(strlen(temp->text) * sizeof(char), GFP_KERNEL);

		msgOut = node->text;
		// strcpy(node->text, msg);

		if (copy_to_user(buffer, (msgOut), strlen(msgOut)) > 0)
		{
			mutex_unlock(&devLock);
			return -EFAULT; /* copy failed */
		}
		// mutex_unlock(&devLock);
		//	result = copy_to_user(buffer, (msgOut), strlen(msgOut));
		// mutex_lock(&devLock);
		list_del(&(node->list));

		if (msgCount == 0)
		{
			// do nothing
		}
		else
		{
			msgCount = msgCount - 1;
		}
		// kfree(msgOut);
		// mutex_unlock(&devLock);
	}

	/*
	 * Most read functions return the number of bytes put into the buffer
	 */
	// kfree(temp->text);
	// kfree(temp);
	mutex_unlock(&devLock);
	return length;
}

/* Called when a process writes to dev file: echo "hi" > /dev/hello  */
static ssize_t
device_write(struct file *filp, const char *buff, size_t len, loff_t *off)
{
	msgIn = (char *)kmalloc(sizeof(char) * BUF_LEN, GFP_KERNEL);
	// static struct k_list *node;

	if (len > 4096)
	{
		return -EINVAL;
	}
	else if (msgCount >= 1000)
	{
		return -EBUSY;
	}
	else
	{

		// printk(KERN_INFO "Allocated %zu bytes of memory", sizeof(msgIn));
		// printk(KERN_INFO "length of message: %zu", len);
		// printk("size of buffer: %zu", sizeof(&buff));

		if (copy_from_user(msgIn, buff, len) > 0)
		{
			kfree(msgIn);
			return -EFAULT;
		}
		mutex_lock(&devLock);
		// temp = copy_from_user(msgIn, buff, len);

		node = kmalloc(sizeof(struct k_list), GFP_KERNEL);
		node->text = msgIn;

		list_add_tail(&node->list, head);
		msgCount = msgCount + 1;
		mutex_unlock(&devLock);
	}
	// kfree(msgIn);
	return len;
}
