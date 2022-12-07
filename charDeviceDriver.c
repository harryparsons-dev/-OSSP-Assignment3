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

MODULE_LICENSE("GPL");

typedef struct node
{
	char *text;
	struct node *next;
} node;

node *head;
head = NULL;
// node *head = NULL;

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

// static long device_ioctl(struct file *file,		 /* see include/linux/fs.h */
// 						 unsigned int ioctl_num, /* number and param for ioctl */
// 						 unsigned long ioctl_param)
// {

// 	/*
// 	 * Switch according to the ioctl called
// 	 */

// 	/* 	    return 0; */
// 	/* can pass integer as return value */
// 	return 0;
// 	/* no operation defined - return failure */
// }

/*
 * This function is called when the module is loaded
 */
// char **strArr()
// {
// 	char **arr = malloc(sizeof(char *) * 1000);
// 	for (int i = 0; i < 1000; i++)
// 	{
// 		arr[i] = malloc(4 * 1024);
// 	}
// 	return arr;
// }

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

	return SUCCESS;
}

/*
 * This function is called when the module is unloaded
 */
void cleanup_module(void)
{
	/*  Unregister the device */
	unregister_chrdev(Major, DEVICE_NAME);
}

/*
 * Methods
 */

node *create_node(int data)
{
	node *new_node = kmalloc(sizeof(node), GFP_KERNEL);
	new_node->data = data;
	new_node->next = NULL;
	return new_node;
}

void insert_node(node **head, node *new_node)
{
	new_node->next = *head;
	*head = new_node;
}

node *remove_node(node **head)
{
	node *removed_node;
	remove_node = *head;
	*head = (*head)->next;
	return removed_node;
}
void free_list(node *head)
{
	node *current = head;
	while (current != NULL)
	{
		node *temp = current;
		current = current->next;
		kfree(temp);
	}
}

/*
 * Called when a process tries to open the device file, like
 * "cat /dev/mycharfile"
 */

static int device_open(struct inode *inode, struct file *file)
{

	mutex_lock(&devLock);
	if (Device_Open)
	{
		mutex_unlock(&devLock);
		return -EBUSY;
	}
	Device_Open++;
	mutex_unlock(&devLock);
	// sprintf(msg, "I already told you %d times Hello world!\n", counter++);
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

	/*
	 * Actually put the data into the buffer
	 */
	if (strlen(msg) + 1 < length)
		length = strlen(msg) + 1;
	result = copy_to_user(buffer, msg, length);
	msgCount = msgCount - 1;
	if (result > 0)
		return -EFAULT; /* copy failed */
	/*
	 * Most read functions return the number of bytes put into the buffer
	 */
	return length;
}

/* Called when a process writes to dev file: echo "hi" > /dev/hello  */
static ssize_t
device_write(struct file *filp, const char *buff, size_t len, loff_t *off)
{
	if ((msgCount < 1000) && ((sizeof(char) * len) < 4 * 1024))
	{
		insert_node(&head, create_node(1));
		sprintf(msg, "%s(%zu letters)", buff, len);
		printk(KERN_INFO "Recived message length: %zu from user", len);
		msgCount = msgCount + 1;
		return len;
	}
	else if (msgCount >= 1000)
	{
		return -EBUSY;
	}
	else
	{
		return -EINVAL;
	}
}
