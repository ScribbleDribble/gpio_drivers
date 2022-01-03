#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
// #include <linux/gpio/driver.h>
#include <linux/gpio.h>



MODULE_LICENSE("GPL");


static char buffer[255];
// we keep a buffer pointer which is indexed where the last character is in the array. To ensure we dont go out of bounds
static int buffer_ptr = 0;

static char value;
// variables for device and device class

// dev_t is a type for holding minor and majour device numbers.     
static dev_t my_device_nr;
// this is our type of device (autdio, networking, etc)
static struct class* my_class;
// used to represent the character device. 
static struct cdev my_device;

#define DRIVER_NAME "gpio_driver"
#define DRIVER_CLASS "MyModuleClass"

static const int gpio_pin_write = 4;

/**
 * @brief read data out of the buffer
 * 
 */


static ssize_t driver_read(struct file* f, char* user_buffer, size_t count, loff_t* offset) {
    int to_copy, not_copied, delta;
    
    char tmp[3] = {value,'\n'};
    
    // get amount of data to copy. 
    

    // this func returns n_bytes that was not copied. arg1: destination address in user space, buffer in kernel, amount to copy in bytes
    not_copied = copy_to_user(user_buffer, tmp, to_copy);

    // this returns the amount of data that was not copied from what we we allowed to copy (to_copy bytes). Essentially
    // worst case is where delta = 0 as it means nothing was copied. 
    delta = to_copy - not_copied;
    return delta;
}

/**
 * @brief write data to the buffer
 * 
 */

static ssize_t driver_write(struct file* f, const char* user_buffer, size_t count, loff_t* offset) {

    int to_copy = 1, not_copied, delta;

    not_copied = copy_from_user(&value, user_buffer, to_copy);

    switch(value) {
        case '0':
            gpio_set_value(gpio_pin_write, 0);
            break;
        case '1':
            gpio_set_value(gpio_pin_write, 1);
            break;
        default:
            return "Invalid input";
    }

    return to_copy - not_copied;
    // this func returns n_bytes that was not copied. arg1: destination address in user space, buffer in kernel, amount to copy in bytes

}


/**
 * @brief 
 * This function is called when the module's device file is opened
 * 
 * @param device_file 
 * @param instance 
 * @return int 
 */
static int driver_open(struct inode* device_file, struct file* instance) {
    printk("open was called!\n");
    return 0;
}

/**
 * @brief This function is called when the module's device file is closed
 * 
 * @param driver_file 
 * @param instance 
 * @return int 
 */
static int driver_close(struct inode* driver_file, struct file* instance) {
    printk("close was called!\n");
    return 0;
}


 
// definition of our device file
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = driver_open,
    .release = driver_close,
    .read = driver_read,
    .write = driver_write
};

static int __init ModuleInit(void) {
    // now we will use this function to automatically register a device number and create a device file. so we need to 
    
    // 1. allocate a device number to the module - my_device_nr
    // 2. set a driver class type - myclass
    // 3. bind the chr device representation to our file operations folder. 
    // 4. initialise the internal representation of our character device - my_device


    int retval;
    printk("Hello Kernel");

    // allocate a device number
    if ( alloc_chrdev_region(&my_device_nr, 0, 1, DRIVER_NAME) < 0 ) 
    {
        printk("Device number could not be allocated\n");
        return -1;
    }
    printk("read_write - Device nr. major: %d, minor: %d was registered\n", my_device_nr >> 20, my_device_nr && 0xfffff);

    // create a device class
    my_class = class_create(THIS_MODULE, DRIVER_CLASS);

    if ( my_class == NULL)
    {
        printk("Device class could not be created\n");
        goto ClassError;    
    }

    
    // create device file 
    if (device_create(my_class, NULL, my_device_nr, NULL, DRIVER_NAME) == NULL) 
    {
        printk("Device file could not be created.\n");
        goto FileError; // gotos are acceptable in some cases within driver programming. 
        // for example, as we have failed on the 2nd stage, we need to clean up from the first stage too (by destroying the created class)
        // so we send this to a point where we can destroy the class and device. 
        // So essentially, we dont have to re-write clean-up code, which builds up for every stage we have passed. 
    }

    // register a char device structure
    cdev_init(&my_device, &fops);

    // now that we have created a device file, we need to initialise it. this adds our character device to the system. 
    if ( cdev_add(&my_device, my_device_nr, 1) == -1 )
    {
        printk("Character device could not be registered.");
        goto AddError;
    }
    return 0;


    // now we need to initialise GPIO pin 4
    if (gpio_request(gpio_pin_write, "rpi-gpio-4"))
    {
        printk("Request failed: rpi-gpio-4");
        goto AddError;
    }

    // we can release the pin using gpio_free
    if (gpio_direction_output(gpio_pin_write, 0) == -1) 
    {
        printk("Failed to output to GPIO PIN 4");
    }

Gpio4Error:
    gpio_free(gpio_pin_write);

AddError:
    device_destroy(my_class, my_device_nr);

FileError:
    class_destroy(my_class);

ClassError:
    unregister_chrdev(my_device_nr, DRIVER_NAME); 

    return -1;
}



static void __exit ModuleExit(void) {
    
    gpio_set_value(gpio_pin_write,0);
    gpio_free(gpio_pin_write);
    // release device number if module is unloaded from the kernel
    // we want to perform clean up from a top down approach (tear down in the opposite order to the way we set things up - see the clean up from module_init for guidance)
    cdev_del(&my_device);
    device_destroy(my_class, my_device_nr);
    class_destroy(my_class);
    unregister_chrdev_region(my_device_nr, 1);

    printk("Goodbye Kernel");
}

module_init(ModuleInit);
module_exit(ModuleExit);