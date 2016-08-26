
#include <linux/init.h>          
#include <linux/module.h>       
#include <linux/device.h>      
#include <linux/kernel.h>     
#include <linux/fs.h>        
#include <asm/uaccess.h>    
#define DEVICE_NAME "char_device_driver"   
#define CLASS_NAME "char_dev" 

MODULE_LICENSE("GPL");
static int    majNum;                 
static char   message[256] = {0};          
static short  size_of_message;            
static int    numberOpens = 0;           
static struct class*  char_device_driverClass  = NULL;
static struct device* char_device_driverDevice = NULL;

static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations fops =
{
   .open = dev_open,
   .read = dev_read,
   .write = dev_write,
   .release = dev_release,
};


//Initialising device and attempting to add Major number
static int __init char_device_driver_init(void){
   printk(KERN_INFO "Initialising...\n");

   // Try to dynamically allocate a major number for the device -- more difficult but worth it
   majNum = register_chrdev(0, DEVICE_NAME, &fops);
   if (majNum<0){
      printk(KERN_ALERT "Failed to register a major number\n");
      return majNum;
   }
   printk(KERN_INFO "Successfully major number %d\n", majNum);

   // Register the device class
   char_device_driverClass = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(char_device_driverClass)){                // Check for error and clean up if there is
      unregister_chrdev(majNum, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register device class\n");
      return PTR_ERR(char_device_driverClass);          // Correct way to return an error on a pointer
   }
   printk(KERN_INFO "EBBChar: device class registered correctly\n");

   // Register the device driver
   char_device_driverDevice = device_create(char_device_driverClass, NULL, MKDEV(majNum, 0), NULL, DEVICE_NAME);
   if (IS_ERR(char_device_driverDevice)){               // Clean up if there is an error
      class_destroy(char_device_driverClass);           // Repeated code but the alternative is goto statements
      unregister_chrdev(majNum, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create the device\n");
      return PTR_ERR(char_device_driverDevice);
   }
   printk(KERN_INFO "EBBChar: device class created correctly\n"); // Made it! device was initialized
   return 0;
}

/** @brief The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required.
 */
static void __exit char_device_driver_exit(void){
   device_destroy(char_device_driverClass, MKDEV(majNum, 0));     // remove the device
   class_unregister(char_device_driverClass);                          // unregister the device class
   class_destroy(char_device_driverClass);                             // remove the device class
   unregister_chrdev(majNum, DEVICE_NAME);             // unregister the major number
   printk(KERN_INFO "EBBChar: Goodbye from the LKM!\n");
}

/** @brief The device open function that is called each time the device is opened
 *  This will only increment the numberOpens counter in this case.
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_open(struct inode *inodep, struct file *filep){
   numberOpens++;
   printk(KERN_INFO "EBBChar: Device has been opened %d time(s)\n", numberOpens);
   return 0;
}

/** @brief This function is called whenever device is being read from user space i.e. data is
 *  being sent from the device to the user. In this case is uses the copy_to_user() function to
 *  send the buffer string to the user and captures any errors.
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 *  @param buffer The pointer to the buffer to which this function writes the data
 *  @param len The length of the b
 *  @param offset The offset if required
 */
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){
   int error_count = 0;
   // copy_to_user has the format ( * to, *from, size) and returns 0 on success
   error_count = copy_to_user(buffer, message, size_of_message);

   if (error_count==0){            // if true then have success
      printk(KERN_INFO "EBBChar: Sent %d characters to the user\n", size_of_message);
      return (size_of_message=0);  // clear the position to the start and return 0
   }
   else {
      printk(KERN_INFO "EBBChar: Failed to send %d characters to the user\n", error_count);
      return -EFAULT;              // Failed -- return a bad address message (i.e. -14)
   }
}

/** @brief This function is called whenever the device is being written to from user space i.e.
 *  data is sent to the device from the user. The data is copied to the message[] array in this
 *  LKM using the sprintf() function along with the length of the string.
 *  @param filep A pointer to a file object
 *  @param buffer The buffer to that contains the string to write to the device
 *  @param len The length of the array of data that is being passed in the const char buffer
 *  @param offset The offset if required
 */
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
   sprintf(message, "%s(%d letters)", buffer, len);   // appending received string with its length
   size_of_message = strlen(message);                 // store the length of the stored message
   printk(KERN_INFO "EBBChar: Received %d characters from the user\n", len);
   return len;
}

/** @brief The device release function that is called whenever the device is closed/released by
 *  the userspace program
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_release(struct inode *inodep, struct file *filep){
   printk(KERN_INFO "EBBChar: Device successfully closed\n");
   return 0;
}

/** @brief A module must use the module_init() module_exit() macros from linux/init.h, which
 *  identify the initialization function at insertion time and the cleanup function (as
 *  listed above)
 */
module_init(char_device_driver_init);
module_exit(char_device_driver_exit);
