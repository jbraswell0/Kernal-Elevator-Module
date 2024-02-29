#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/ktime.h>
#include <linux/timekeeping.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("group19");
MODULE_DESCRIPTION("Obtain the current time and store it in the module.");
MODULE_VERSION("1.0");

#define ENTRY_NAME "timer"
#define PERMS 0666
#define PARENT NULL

#define BUF_LEN 100

static struct proc_dir_entry* proc_entry;
static char msg[BUF_LEN];
static int procfs_buf_len;

static ssize_t procfile_read(struct file* file, char* ubuf, size_t count, loff_t *ppos)
{
    printk(KERN_INFO "proc_read\n");
    procfs_buf_len = strlen(msg);
    if (*ppos > 0 || count < procfs_buf_len)
        return 0;

    struct timespec64 time;

    ktime_get_real_ts64(&time);
    printk(KERN_INFO "Current time: %lld.%09ld\n", (long long)time.tv_sec, time.tv_nsec);

    if (copy_to_user(ubuf, msg,procfs_buf_len))
        return -EFAULT;
    *ppos = procfs_buf_len;
    printk(KERN_INFO "gave to user %s\n", msg);

    return procfs_buf_len;
}



static ssize_t procfile_write(struct file* file, const char* ubuf, size_t count, loff_t* ppos) {
    printk(KERN_INFO "proc_write\n");
    if (count > BUF_LEN)
        procfs_buf_len = BUF_LEN;
    else
        procfs_buf_len = count;

    if (copy_from_user(msg, ubuf, procfs_buf_len)) {
        printk(KERN_WARNING "Failed to copy data from user space\n");
        return -EFAULT;
    }
    printk(KERN_INFO "got from user: %s\n", msg);
    return procfs_buf_len;
}

static const struct proc_ops procfile_fops = {
    .proc_read = procfile_read,
    .proc_write = procfile_write,
};

static int __init my_timer_init(void){
    proc_entry = proc_create(ENTRY_NAME, PERMS, PARENT, &procfile_fops);
    if (proc_entry == NULL)
        return -ENOMEM;
    return 0;
}


static void __exit my_timer_exit(void){
    proc_remove(proc_entry);
}


module_init(my_timer_init);
module_exit(my_timer_exit);
