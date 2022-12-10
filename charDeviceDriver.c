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

MODULE_LICENSE("GPL");

struct k_list
{
	struct list_head list;
	char *text;
};

static LIST_HEAD(Head_node);
struct list_head *head;
static struct k_list *node;

int temp;
// char *msg;
//  INIT_LIST_HEAD(Head_node);
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
	kfree(msg);
	unregister_chrdev(Major, DEVICE_NAME);
}

/*
 * Methods
 */

// void insert_msg(node *head, char *msg)
// {

// 	node *new_node = (node *)kmalloc(sizeof(node), GFP_KERNEL);
// 	// node *curr_ptr = head;

// 	new_node->text = msg;
// 	new_node->next = NULL;
// 	if (head == NULL)
// 	{
// 		head->next = new_node;
// 	}
// 	else
// 	{
// 		node *curr_ptr = head;

// 		while (curr_ptr->next != NULL)
// 		{
// 			// memcpy(current->next, current, sizeof(node));
// 			curr_ptr = curr_ptr->next;
// 		}
// 		curr_ptr->next = new_node;
// 	}
// 	return;
// }

// node *getFirst(node *head)
// {
// 	head = head->next;
// 	// head = *head->next;
// 	// kfree(head);
// 	return head;
// }

// node *remove_node(node **head)
// {
// 	node *removed_node;
// 	remove_node = *head;
// 	*head = (*head)->next;
// 	return removed_node;
// }
// void free_list(node *head)
// {
// 	node *current = head;
// 	while (current != NULL)
// 	{
// 		node *temp = current;
// 		current = current->next;
// 		kfree(temp);
// 	}
// }

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
	int result;
	int count = 0;
	struct k_list *temp;
	// char *msg2 = MSG->text;

	// strcpy(msg2, MSG->text);
	/*
	 * Actually put the data into the buffer
	 */
	mutex_lock(&devLock);
	if (strlen(msg) + 1 < length)
	{
		length = strlen(msg) + 1;
	}
	// node *q = list_first_entry(&msgStore, node, list);
	//  printk(KERN_INFO "From the list {%s}", q->text);

	list_for_each_entry(temp, head, list)
	{
		printk(KERN_INFO "Node %d data = %s\n", count++, temp->text);
	}
	printk(KERN_INFO "Total Nodes = %d\n", count);
	if (list_empty(head))
	{
		mutex_unlock(&devLock);
		return -EAGAIN;
	}
	else
	{

		node = list_first_entry(head, struct k_list, list);
		msg = node->text;
		result = copy_to_user(buffer, msg, length);
		list_del(&node->list);
		if (msgCount == 0)
		{
			// do nothing
		}
		else
		{
			msgCount = msgCount - 1;
		}
	}

	if (result > 0)
		return -EFAULT; /* copy failed */
	/*
	 * Most read functions return the number of bytes put into the buffer
	 */
	mutex_unlock(&devLock);
	return length;
}

/* Called when a process writes to dev file: echo "hi" > /dev/hello  */
static ssize_t
device_write(struct file *filp, const char *buff, size_t len, loff_t *off)
{

	if (sizeof(buff) > 1024)
	{
		return -EINVAL;
	}
	else if (msgCount >= 1000)
	{
		return -EBUSY;
	}
	else
	{
		mutex_lock(&devLock);
		msg = kmalloc(10 * sizeof(char), GFP_KERNEL);
		temp = copy_from_user(msg, buff, len);
		//  struct node *temp_node = NULL;
		node = kmalloc(sizeof(struct k_list), GFP_KERNEL);
		node->text = msg;
		// INIT_LIST_HEAD(&temp_node->list);
		list_add_tail(&node->list, head);

		msgCount = msgCount + 1;

		mutex_unlock(&devLock);
	}

	return len;
	// return -1;

	// return -EINVAL;
}
