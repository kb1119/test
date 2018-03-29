#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <asm/current.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/kdev_t.h>
#include <linux/gpio.h>
#include <linux/errno.h>
#include <uapi/asm-generic/errno-base.h>
#include <linux/string.h>
#include <linux/spinlock.h>
#include <linux/time.h>


#include <linux/syslog.h>
#include <linux/signal.h>

/*------s5P4418 GPIO Register value---------------------*/
/*------s5P4418 GPIO Register value---------------------*/


/*debug switch ---------------------------------------*/
#define  DEVICE_WRITE
#define  DEVICE_READ
#define  DEVICE_CLOSE
#define  DEVICE_INTERRUPT

/*-------------device information-------------------------*/
#define DEVICE_NAME "Artik-gpio"
#define NUM_BUFFER 256
#define NUM_GPIO_PINS 1
#define BUF_SIZE 512
#define INTERRUPT_DEVICE_NAME "Artik-gpio-interrupt"
#define GPIO_PIN 32
/*user defined state type*/
enum state {low, hight};
enum direction {in, out};

/*gpio struct*/
struct gpio_dev {
     struct cdev cdev;
     struct gpio pin;
     enum state state;
     enum direction dir;
     bool irq_perm;
     unsigned long irq_flag;
     unsigned int irq_counter;
     spinlock_t lock;
};

struct gpio_dev *gpio_devp;
/*the number of the devices that could be used */
static const unsigned int MINOR_BASE = 0;

/* device object in class */
static struct class *mydevice_class = NULL;

/*??????*/
static dev_t first;

/*the handler table that communicates with systemcalls*/

static int mydevice_init(void);
static void mydevice_exit(void);
static int mydevice_open(struct inode *inode, struct file *filep);
static int mydevice_close(struct inode *inode, struct file *filep);
static ssize_t mydevice_read(struct file *filep, char __user *buf, size_t count, loff_t *f_pos);
static ssize_t mydevice_write(struct file *filep, const char __user *buf, size_t count, loff_t *f_pos);

struct file_operations s_mydevice_fops = {
            .owner     =  THIS_MODULE,
            .open      =  mydevice_open,
            .release   =  mydevice_close,
            .read      =  mydevice_read,
            .write     =  mydevice_write,
};


//static char stored_value[NUM_BUFFER];
struct _mydevice_file_data{
  unsigned char buffer[NUM_BUFFER];
};
/*-------------device information-------------------------*/


/*interrupt-----------------------------------------------*/



#ifdef DEVICE_INTERRUPT


static const char * const shutdown_argv[]=
      {"/usr/sbin/shutdown", "-h", "-P", "now", NULL};


static irqreturn_t irq_handler(int irq, void *arg){
   
         unsigned long flags;

       local_irq_save(flags);
       printk(KERN_NOTICE "Interrupt [%d] was triggered \n", irq);
  
    call_usermodehelper(shutdown_argv[0], shutdown_argv, NULL, UMH_NO_WAIT);
    return IRQ_HANDLED;
}
#endif




/* the function that is called when it's opened" */
static int mydevice_open(struct inode *inode, struct file *filep)
{

    unsigned int gpio;

    printk(KERN_INFO "GPIOstart open \n");
    gpio = iminor(inode);
    printk(KERN_INFO "GPIO[%d] opend \n", gpio);
    gpio_devp = container_of (inode->i_cdev, struct gpio_dev, cdev);
    filep->private_data = gpio_devp;


    return 0;

}



#ifdef DEVICE_CLOSE
/* the function that is called when it's closed" */
static int mydevice_close(struct inode *inode, struct file *filep)
{

     printk(KERN_INFO "Artik-gpio Driver close\n");
      
     return 0;

}

#endif

/* the function that is called when it's read" */

#ifdef DEVICE_READ
static ssize_t mydevice_read(struct file *filep, char __user *buf, size_t count, loff_t *f_pos)
{

          unsigned int gpio;
          char byte;

          printk(KERN_DEBUG "Artik-gpio Driver read\n");
          gpio = GPIO_PIN;
          byte = '0' + gpio_get_value(gpio);
          put_user(byte, buf);

          return 1;
}


#endif

#ifdef DEVICE_WRITE
/* the function that is called when it's written" */
static ssize_t mydevice_write(struct file *filep, const char __user *buf, size_t count, loff_t *f_pos)
{

      unsigned int gpio, len = 0, value = 0;
      char kbuf[BUF_SIZE];
      unsigned long flags;

     printk(KERN_DEBUG "Artik-gpio Driver write\n");
     gpio= GPIO_PIN;
     len = count < BUF_SIZE ? count - 1 : BUF_SIZE -1;
     if(copy_from_user(kbuf, buf, len) != 0)
     return -EFAULT;
     kbuf[len] = '\0';
     printk(KERN_INFO "Request from user : %s\n", kbuf);


  //gpio direction setting
     if(strcmp(kbuf, "in") == 0){
         if (gpio_devp->dir != in){
             printk(KERN_INFO "Set gpio [%d] direction : input\n", gpio);
//             spin_lock_irqsave(&gpio_devp->lock, flags);
             gpio_direction_input(gpio); 
             gpio_devp->dir = in;
  //           spin_unlock_irqrestore(&gpio_devp->lock, flags);
         }else{

             printk(KERN_INFO "GPIO is already set as input\n");
    //         spin_lock_irqsave(&gpio_devp->lock, flags);
    //         spin_unlock_irqrestore(&gpio_devp->lock, flags);
        } 


             printk(KERN_DEBUG "we are in mydevice_write in if loop\n");
    }

#ifdef DEVICE_INTERRUPT
//interrupt trigger edge setting
    if((strcmp(kbuf, "rising") == 0) || (strcmp(kbuf, "falling") == 0)){
           spin_lock_irqsave(&gpio_devp->lock, flags);
           gpio_direction_input(gpio);
           gpio_devp->dir = in;
           gpio_devp->irq_perm = true;
            if( strcmp(kbuf, "rising") == 0)
                 gpio_devp->irq_flag = IRQF_TRIGGER_RISING;
            else gpio_devp->irq_flag = IRQF_TRIGGER_FALLING;
  //         spin_unlock_irqrestore(&gpio_devp->lock, flags);
    }else if (strcmp(kbuf, "disable-irq") == 0){
      //     spin_lock_irqsave(&gpio_devp->lock, flags);
           gpio_devp->irq_perm = false;
    //       spin_unlock_irqrestore(&gpio_devp->lock, flags);
           }else printk(KERN_DEBUG "we are in .write interrupt setting");
                 
#endif
     return count;
}
#endif


/* the function that is called when it's loaded" */
static int __init mydevice_init(void)
{
       int i =0, ret;
       unsigned int gpio = GPIO_PIN;
       int err, irq ;
       unsigned long flags;


       printk(KERN_DEBUG "start GPIO init\n");

// character type device registration
if (alloc_chrdev_region( 
      &first,            //the first device number
      MINOR_BASE,        // the start value of Minor number
      NUM_GPIO_PINS,     // Number of devices that you want to get
      DEVICE_NAME        // Module name that is shown up in /proc/devices
       ) < 0){
        printk(KERN_DEBUG "Cannot register device \n");
        return -1;
      }

//Registration of this module in Class /sys/class/DEVICE_NAME
if((mydevice_class = class_create(
    THIS_MODULE,
    DEVICE_NAME
    )) == NULL){
         printk(KERN_DEBUG "Cannot create class %s\n", DEVICE_NAME);
         unregister_chrdev_region(first, NUM_GPIO_PINS);
         return -EINVAL;
         }

gpio_devp = kmalloc(sizeof(struct gpio_dev), GFP_KERNEL);
if (!gpio_devp){
     printk("Bad kmalloc\n");
     return -ENOMEM;
}

if (gpio_request_one(
                      gpio,// GPIO32 = SW404 on artik530 demo board      
                      GPIOF_OUT_INIT_HIGH,
                      NULL
                    ) < 0){
                 printk(KERN_ALERT "Error Requesting GPIO %d\n", i);
                 return -ENODEV;
                 }
   gpio_devp->dir = in;
   gpio_devp->state = low;
   gpio_devp->cdev.owner = THIS_MODULE;

   spin_lock_init(&gpio_devp->lock);

//initialization of Char type device
//fileoperation 
  cdev_init(&gpio_devp->cdev, &s_mydevice_fops);


//registration of this module in Kernel
   if ((ret = cdev_add(&gpio_devp->cdev, (first + i), 1))){
       printk (KERN_ALERT "Error %d adding cdev\n", ret);
       device_destroy(mydevice_class, MKDEV(MAJOR(first), MINOR(first + i)));
       class_destroy(mydevice_class);
       unregister_chrdev_region(first, NUM_GPIO_PINS);
       return ret;
     }

   //according to minor number 
   if (device_create(mydevice_class, NULL, MKDEV(MAJOR(first), MINOR(first) + i), NULL, "Artik-gpio%d", i ) == NULL){
       class_destroy(mydevice_class);
       unregister_chrdev_region(first, NUM_GPIO_PINS);
       return -1;
    }


//Interrupt registration
#ifdef DEVICE_INTERRUPT 

    gpio_devp->irq_perm = true;
    gpio_devp->irq_flag = IRQF_TRIGGER_FALLING;


    if((gpio_devp->irq_perm == true) && (gpio_devp->dir == in)) {
        if((gpio_devp->irq_counter++ == 0)){
            irq = gpio_to_irq(gpio);
             if(gpio_devp->irq_flag == IRQF_TRIGGER_RISING) {
      //          spin_lock_irqsave(&gpio_devp->lock, flags);
                err = request_irq (irq, irq_handler, IRQF_SHARED | IRQF_TRIGGER_RISING, INTERRUPT_DEVICE_NAME, gpio_devp);
                printk(KERN_INFO "Interupt requested\n");
    //            spin_unlock_irqrestore(&gpio_devp->lock, flags);
             }else{
  //              spin_unlock_irqrestore(&gpio_devp->lock, flags);
                err = request_irq (irq, irq_handler, IRQF_SHARED | IRQF_TRIGGER_FALLING, INTERRUPT_DEVICE_NAME, gpio_devp);
                printk(KERN_INFO "Interupt requested\n");
//                spin_unlock_irqrestore(&gpio_devp->lock, flags);
             }
             if (err != 0 ){
                printk(KERN_INFO "unable to claim irq: %d, error %d\n", irq, err);
                return err;
             }
            }
       }
#endif




  printk("Artik_GPIO Driver initialized\n");
  return 0;
}


/* the function that is called when it's unloaded" */
static void __exit mydevice_exit(void)
{

   unsigned int gpio = GPIO_PIN;
    printk("Arti_GPIO Driver exit\n");

#ifdef DEVICE_INTERRUPT
   int irq = gpio_to_irq(gpio);
   free_irq(irq, irq_handler);
#endif


   unregister_chrdev_region(first, NUM_GPIO_PINS);
   kfree(gpio_devp);
   device_destroy(mydevice_class, MKDEV(MAJOR(first), MINOR(first) + 1));
   gpio_free(gpio);
   class_destroy(mydevice_class);
   printk(KERN_INFO "Artik_GPIO_driver removed \n");

}

module_init(mydevice_init);
module_exit(mydevice_exit);


MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("hirokazu.kobayashi@eurotech.com");
MODULE_DESCRIPTION("Artik GPIO Device Driver");

























