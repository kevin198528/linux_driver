#include <linux/module.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#define GLOBALMEM_SIZE  0x1000
#define MEM_CLEAR       0x1
#define GLOBALMEM_MAJOR 230

#define u8 unsigned char
#define u32 unsigned int
#define u64 unsigned long

static int globalmem_major = GLOBALMEM_MAJOR
module_param(globalmem_major, int, S_IRUGO);

typedef struct globalmem_dev {
   struct cdev cdev;
   u8 mem[GLOBALMEM_SIZE];    
}GM_DEV;

struct globalmem_dev * g_dev_p;

static int gm_open(struct inode * inode, struct file * file_p)
{
   file_p->private_data = g_dev_p;
}

static int gm_release(struct inode * inode, struct file * file_p)
{
   return 0;
}

static long gm_ioctl(struct file * file_p, u32 cmd, u64 arg)
{
   GM_DEV * dev = file_p->private_data;

   switch (cmd) {
   case MEM_CLEAR:
      memset(dev->mem, 0, GLOBALMEM_SIZE);   
      printk(KERN_INFO "globalmem is set to zero\n");
      break;

   default:
      return -EINVAL;
   }

   return 0;
}

static ssize_t gm_read(struct file * file_p, char __user * buff, size_t size, loff_t * ppos)
{
   u64 p = *ppos;
   u32 count = size;
   int ret = 0;
   GM_DEV * dev = file_p->private_data;

   if (p > GLOBALMEM_SIZE) {
      ret = 0;
      goto err;
   }

   if (count > GLOBALMEM_SIZE - p)
      count = GLOBALMEM_SIZE - p;

   if (copy_to_user(buff, dev->mem + p, count))
      goto err;
   
   *ppos += count;
   ret = count;
   printk(KERN_INFO "read %u bytes(s) from %lu\n", count, p);
   
   return ret;
err:
   ret = -EFAULT;
   return ret;
}

static ssize_t gm_write(struct file * file_p, const char __user * buff, size_t size, loff_t * ppos)
{
   u64 p = *ppos;
   u32 count = size;
   int ret = 0;
   GM_DEV * dev = file_p->private_data;

}

static void globalmem_setup_cdev(struct globalmem_dev * dev, int index)
{
   int err, devno = MKDEV(globalmem_major, index);
   
   cdev_init(&dev->cdev, &globalmem_fops);
   dev->cdev.owner = THIS_MODULE;
   err = cdev_add(&dev->cdev, devno, 1);
   if (err)
      printk(KERN_NOTICE "error %d adding globalmem %d\n", err, index);
}

static int __init globalmem_init(void)
{
   int ret;
   dev_t devno = MKDEV(globalmem_major, 0);

   if (globalmem_major)
      ret = register_chrdev_region(devno, 1, "globalmem");
   else {
      ret = alloc_chrdev_region(&devno, 0, 1, "globalmem");
      globalmem_major = MAJOR(devno);
   }

   if (ret < 0)
      return ret;

   globalmem_devp = kzalloc(sizeof(struct globalmem_dev), GFP_KERNEL);
}


