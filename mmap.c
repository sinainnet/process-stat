#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/init.h> 
#include <linux/fs.h> 
#include <linux/mm.h> 
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/pid.h>
#include <linux/sched/mm.h>

#define MAX_SIZE (PAGE_SIZE * 10)   /* max size mmaped to userspace */
#define DEVICE_NAME "mchar"
#define  CLASS_NAME "mogu"

struct memory_stat_report
{
	unsigned long vmpeak;
	unsigned long vmsize;
	unsigned long vmlck;
	unsigned long vmpin;
	unsigned long vmhwm;
	unsigned long vmrss;
	unsigned long rssanon;
	unsigned long rssfile;
	unsigned long rssshmem;
	unsigned long vmdata;
	unsigned long vmstk;
	unsigned long vmexe;
	unsigned long vmlib;
	unsigned long vmpte;
	unsigned long vmswap;
	unsigned long hugetlb;
};

static struct class*  class;
static struct device*  device;
static int major;
static char *sh_mem = NULL; 
static int given_task_pid = -1;
static struct task_struct * given_task = NULL;

static DEFINE_MUTEX(mchar_mutex);

/*  executed once the device is closed or releaseed by userspace
 *  @param inodep: pointer to struct inode
 *  @param filep: pointer to struct file 
 */
static int mchar_release(struct inode *inodep, struct file *filep)
{    
    mutex_unlock(&mchar_mutex);
    pr_info("mchar: Device successfully closed\n");

    return 0;
}

/* executed once the device is opened.
 *
 */
static int mchar_open(struct inode *inodep, struct file *filep)
{
    int ret = 0; 

    if(!mutex_trylock(&mchar_mutex)) {
        pr_alert("mchar: device busy!\n");
        ret = -EBUSY;
        goto out;
    }
 
    pr_info("mchar: Device opened\n");

out:
    return ret;
}

/*  mmap handler to map kernel space to user space  
 *
 */
static int mchar_mmap(struct file *filp, struct vm_area_struct *vma)
{
    int ret = 0;
    struct page *page = NULL;
    unsigned long size = (unsigned long)(vma->vm_end - vma->vm_start);

    if (size > MAX_SIZE) {
        ret = -EINVAL;
        goto out;  
    } 
   
    page = virt_to_page((unsigned long)sh_mem + (vma->vm_pgoff << PAGE_SHIFT)); 
    ret = remap_pfn_range(vma, vma->vm_start, page_to_pfn(page), size, vma->vm_page_prot);
    if (ret != 0) {
        goto out;
    }   

out:
    return ret;
}

static ssize_t mchar_read(struct file *filep, char *buffer, size_t len, loff_t *offset)
{
    int ret;

	struct mm_struct* given_task_mm_struct = NULL;

	size_t mm_struct_size = 0;
    
	if (given_task)
	{
		given_task_mm_struct = get_task_mm(given_task);
		if (given_task_mm_struct)
		{
			mm_struct_size = sizeof(*given_task_mm_struct);
			{
				struct mm_struct* mm = given_task_mm_struct;
				unsigned long text, lib, swap, anon, file, shmem;
				unsigned long hiwater_vm, total_vm, hiwater_rss, total_rss;
				anon = get_mm_counter(mm, MM_ANONPAGES);
				file = get_mm_counter(mm, MM_FILEPAGES);
				shmem = get_mm_counter(mm, MM_SHMEMPAGES);
				hiwater_vm = total_vm = mm->total_vm;
				if (hiwater_vm < mm->hiwater_vm)
					hiwater_vm = mm->hiwater_vm;
				hiwater_rss = total_rss = anon + file + shmem;
				if (hiwater_rss < mm->hiwater_rss)
					hiwater_rss = mm->hiwater_rss;

				text = (PAGE_ALIGN(mm->end_code) - (mm->start_code & PAGE_MASK)) >> 10;
				lib = (mm->exec_vm << (PAGE_SHIFT-10)) - text;
				swap = get_mm_counter(mm, MM_SWAPENTS);
				if (sh_mem)
				{
					struct memory_stat_report* msr = (struct memory_stat_report*)sh_mem;
					msr->vmpeak = hiwater_vm << (PAGE_SHIFT-10);
					msr->vmsize = total_vm << (PAGE_SHIFT-10);
					msr->vmlck = mm->locked_vm << (PAGE_SHIFT-10);
					msr->vmpin = mm->pinned_vm << (PAGE_SHIFT-10);
					msr->vmhwm = hiwater_rss << (PAGE_SHIFT-10);
					msr->vmrss = total_rss << (PAGE_SHIFT-10);
					msr->rssanon = anon << (PAGE_SHIFT-10);
					msr->rssfile = file << (PAGE_SHIFT-10);
					msr->rssshmem = shmem << (PAGE_SHIFT-10);
					msr->vmdata = mm->data_vm << (PAGE_SHIFT-10);
					msr->vmstk = mm->stack_vm << (PAGE_SHIFT-10);
					msr->vmexe = text;
					msr->vmlib = lib;
					msr->vmpte = mm_pgtables_bytes(mm) >> 10;
					msr->vmswap = swap << (PAGE_SHIFT-10);
					msr->hugetlb = atomic_long_read(&mm->hugetlb_usage) << (PAGE_SHIFT - 10);
				}
			}
			mmput(given_task_mm_struct);
		}
	}

    if (len > MAX_SIZE) {
        pr_info("read overflow!\n");
        ret = -EFAULT;
        goto out;
    }

	if (len < sizeof(mm_struct_size))
	{
        pr_info("read overflow!\n");
        ret = -EFAULT;
        goto out;
    }

    if (copy_to_user(buffer, &mm_struct_size, sizeof(mm_struct_size)) == 0) {
        pr_info("mchar: copy %u char to the user\n", len);
        ret = sizeof(mm_struct_size);
    } else {
        ret =  -EFAULT;   
    } 

out:
    return ret;
}

static ssize_t mchar_write(struct file *filep, const char *buffer, size_t len, loff_t *offset)
{
    pr_info("mchar: copy %d char from the user\n", len);
	given_task_pid = *(int*)(&buffer[0]);

	given_task = pid_task(find_vpid(given_task_pid), PIDTYPE_PID);

    return sizeof(given_task_pid);
}

static const struct file_operations mchar_fops = {
    .open = mchar_open,
    .read = mchar_read,
    .write = mchar_write,
    .release = mchar_release,
    .mmap = mchar_mmap,
    /*.unlocked_ioctl = mchar_ioctl,*/
    .owner = THIS_MODULE,
};

static int __init mchar_init(void)
{
    int ret = 0;    
    major = register_chrdev(0, DEVICE_NAME, &mchar_fops);

    if (major < 0) {
        pr_info("mchar: fail to register major number!");
        ret = major;
        goto out;
    }

    class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(class)){ 
        unregister_chrdev(major, DEVICE_NAME);
        pr_info("mchar: failed to register device class");
        ret = PTR_ERR(class);
        goto out;
    }

    device = device_create(class, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);
    if (IS_ERR(device)) {
        class_destroy(class);
        unregister_chrdev(major, DEVICE_NAME);
        ret = PTR_ERR(device);
        goto out;
    }


    /* init this mmap area */
    sh_mem = kmalloc(MAX_SIZE, GFP_KERNEL); 
    if (sh_mem == NULL) {
        ret = -ENOMEM; 
        goto out;
    }

    sprintf(sh_mem, "xyz\n");  
    
    mutex_init(&mchar_mutex);
out: 
    return ret;
}

static void __exit mchar_exit(void)
{
    mutex_destroy(&mchar_mutex); 
    device_destroy(class, MKDEV(major, 0));  
    class_unregister(class);
    class_destroy(class); 
    unregister_chrdev(major, DEVICE_NAME);
    
    pr_info("mchar: unregistered!");
}

module_init(mchar_init);
module_exit(mchar_exit);
MODULE_LICENSE("GPL");
