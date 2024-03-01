#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/ktime.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("group19");
MODULE_DESCRIPTION("Obtain the current time and store it in the module.");
MODULE_VERSION("1.0");

#define ENTRY_NAME "timer"
#define PERMS 0666
#define PARENT NULL

#define BUF_LEN 200 // Increased buffer size to accommodate both messages

static struct proc_dir_entry* proc_entry;
static char msg[BUF_LEN];
static int procfs_buf_len;
static struct timespec64 prev_time;

static ssize_t procfile_read(struct file* file, char* ubuf, size_t count, loff_t *ppos)
{
    struct timespec64 time;
    char buf[BUF_LEN];
    long long elapsed_sec, elapsed_nsec;

    ktime_get_real_ts64(&time);

    if (prev_time.tv_sec != 0 || prev_time.tv_nsec != 0) {
        elapsed_sec = time.tv_sec - prev_time.tv_sec;
        elapsed_nsec = time.tv_nsec - prev_time.tv_nsec;

        if (elapsed_nsec < 0) {
            elapsed_sec--;
            elapsed_nsec += 1000000000;
        }

        sprintf(buf, "Current time: %lld.%09ld\nElapsed time since last call: %lld.%09ld seconds\n",
                (long long)time.tv_sec, time.tv_nsec, elapsed_sec, elapsed_nsec);
    } else {
        sprintf(buf, "Current time: %lld.%09ld\nNo previous call recorded.\n",
                (long long)time.tv_sec, time.tv_nsec);
    }

    prev_time = time;

    procfs_buf_len = strlen(buf);

    if (*ppos > 0 || count < procfs_buf_len)
        return 0;

    if (copy_to_user(ubuf, buf, procfs_buf_len))
        return -EFAULT;

    *ppos = procfs_buf_len;

    return procfs_buf_len;
}

static ssize_t procfile_write(struct file* file, const char* ubuf, size_t count, loff_t* ppos) {
    printk(KERN_INFO "proc_write\n");
    return -EPERM; // Disallow writing to /proc/timer
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
