#include <linux/tty.h> 				//allows us to use my.driver
					   				// struct tty.driver defined here
					   				// struct port.tty

#include <linux/kd.h>				//KDSETLED - a constant that is used to tell the keyboard driver that we want to
									//				manipulate the LEDs.

#include <linux/string.h> 			// allows us to use strlen(), strcmp(), strncmp()
#include <linux/module.h>           //This is a LKM (linux kernel module)
#include <linux/fs.h> 				//provides file_operations struct
#include <linux/device.h> 			//gives class_create and device_create
#include <linux/uaccess.h> 			//gives us copy_to_user

#include <linux/console_struct.h> 	// struct vc_cons ** must go last

extern int fg_console;				//allows us to access fg_consol
static struct tty_driver *my_driver;


#define  DEVICE_NAME "ledgo"    	// The device will appear at /dev/ledgo using this value
#define  CLASS_NAME  "led"        	// The device class 

static struct class*  ebbcharClass  = NULL; // The device-driver class struct pointer
static struct device* ebbcharDevice = NULL; // The device-driver device struct pointer

//my_driver = vc_cons[fg_console].d->port.tty->driver;
//int state = 7 //all 3 leds on
			// controls : 
			// bit 2 - caps-lock LED
			// bit 1 - num-lock LED
			// bit 0 - scroll-lock LED


//reads led file and prints out lightbulb
static ssize_t device_read(struct file *filp, char *buff, size_t length, loff_t *offset)
{
	const char *s_ptr;
	int error_code = 0;
	ssize_t len = 0;
	
    //will print out "light bulb" when read
	s_ptr = " - \n(y)\n = \n";
	
	len = min( (unsigned long)(strlen(s_ptr) - *offset) , (unsigned long)(length));
	
	if(len<=0)
	{
		return 0;
	}
	
	error_code = copy_to_user(buff, s_ptr + *offset, len);
	printk(KERN_ALERT "The led file was read! Len: %d \n", len);
	printk(KERN_ALERT "Error Code: %d \n", error_code);
	
	*offset += len;
	return len;
}

// does the action of turning the light on and off and resetting to current actual state
// note : doesn't really work if caps lock already on but you said that's ok ...
static ssize_t device_write(struct file *filep, const char *buff, size_t len, loff_t *offset)
{
	
	//check to see what state and set
    
    //if on
	if (strncmp(buff, "on", 2) ==0 )
	{
		((my_driver->ops)->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED, 4);
	}
    
    //if off
	else if (strncmp(buff, "off",3) == 0)
	{
		((my_driver->ops)->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED, 0);
	}
    
    //if reset to current state
	else if (strncmp(buff, "reset",5) ==0)
	{
		((my_driver->ops)->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED, 15);
	}	


	printk(KERN_INFO "LED: Received %zu characters from the user\n", len);
	return len;
}

//file operations to occur
static struct file_operations fops= {
		.read = device_read,
		.write = device_write,		
};

static int major;

// for start up - replaces mknod
static int __init start(void){
	my_driver = vc_cons[fg_console].d->port.tty->driver;

	printk(KERN_ALERT "LED Module Loaded!\n");
	major = register_chrdev(0, "led", &fops);
	if(major<0){
		printk(KERN_ALERT "Failed to register led file!\n");
		return major; 
	}
	printk(KERN_ALERT "New 'led' created with major %d and minor %d!\n", major, 0);
	
	
	
   // Register the device class
   ebbcharClass = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(ebbcharClass)){                // Check for error and clean up if there is
		unregister_chrdev(major, DEVICE_NAME);
		printk(KERN_ALERT "Failed to register device class\n");
		return PTR_ERR(ebbcharClass);          // Correct way to return an error on a pointer
	}
	printk(KERN_INFO "LED: device class registered correctly\n");
 
   // Register the device driver
	ebbcharDevice = device_create(ebbcharClass, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
	if (IS_ERR(ebbcharDevice)){               // Clean up if there is an error
	  class_destroy(ebbcharClass);           // Repeated code but the alternative is goto statements
	  unregister_chrdev(major, DEVICE_NAME);
	  printk(KERN_ALERT "Failed to create the device\n");
	  return PTR_ERR(ebbcharDevice);
	}
	printk(KERN_INFO "Led: device class created correctly\n");
	
	return 0;
}


static void __exit end(void){
	printk("LED Module 0 !\n");
	device_destroy(ebbcharClass, MKDEV(major, 0)); 		   	 // remove the device
	printk("LED Module 1 !\n");
	class_unregister(ebbcharClass); 
	printk("LED Module 2 !\n");                         // unregister the device class
	class_destroy(ebbcharClass);  
	printk("LED Module 3 !\n");                           // remove the device class
	unregister_chrdev(major, "led");
	printk("LED Module Unloaded!\n");
}

module_init(start);
module_exit(end);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ERIN FAGO");
MODULE_DESCRIPTION("LED controller!!");
