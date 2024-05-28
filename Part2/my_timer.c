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

#define BUF_LEN 200 

static struct proc_dir_entry* proc_entry; 
static int procfs_buf_len; 
static struct timespec64 prev_time; 

// Function that is called when the /proc/timer is read
static ssize_t procfile_read(struct file* file, char* ubuf, size_t count, loff_t *ppos)
{
    struct timespec64 time; // Variable to store the current time
    char buf[BUF_LEN]; // Buffer to store the string to be copied to user space
    long long elapsed_sec, elapsed_nsec; // Variables to store elapsed time

    ktime_get_real_ts64(&time); // Get the current real time

    // Calculate elapsed time if this is not the first read
    if (prev_time.tv_sec != 0 || prev_time.tv_nsec != 0) {
        elapsed_sec = time.tv_sec - prev_time.tv_sec;
        elapsed_nsec = time.tv_nsec - prev_time.tv_nsec;

        // Adjust for negative nanosecond difference
        if (elapsed_nsec < 0) {
            elapsed_sec--;
            elapsed_nsec += 1000000000;
        }

        // Format the time and elapsed time into the buffer
        sprintf(buf, "Current time: %lld.%09ld\nElapsed time since last call: %lld.%09lld seconds\n",
                (long long)time.tv_sec, time.tv_nsec, elapsed_sec, elapsed_nsec);
    } else {
        // If this is the first read, only show the current time
        sprintf(buf, "Current time: %lld.%09ld\n",
                (long long)time.tv_sec, time.tv_nsec);
    }

    // Update the previous time for the next read
    prev_time = time;

    procfs_buf_len = strlen(buf); // Update the buffer length

    // Handle partial reads
    if (*ppos > 0 || count < procfs_buf_len)
        return 0;

    // Copy buffer to user space
    if (copy_to_user(ubuf, buf, procfs_buf_len))
        return -EFAULT;

    *ppos = procfs_buf_len; // Update the position for partial reads

    return procfs_buf_len; // Return the length of the buffer
}

// Function that is called when the /proc/timer is written to
static ssize_t procfile_write(struct file* file, const char* ubuf, size_t count, loff_t* ppos) {
    printk(KERN_INFO "proc_write\n"); // Log that a write attempt was made
    return -EPERM; // Return an error code indicating operation not permitted
}

// Define file operations for the proc file
static const struct proc_ops procfile_fops = {
    .proc_read = procfile_read,
    .proc_write = procfile_write,
};

// Function called when module is loaded
static int __init my_timer_init(void){
    proc_entry = proc_create(ENTRY_NAME, PERMS, PARENT, &procfile_fops); // Create the proc entry
    if (proc_entry == NULL)
        return -ENOMEM; // Return an error if creation failed
    return 0; // Return 0 on successful module initialization
}

// Function called when module is unloaded
static void __exit my_timer_exit(void){
    proc_remove(proc_entry); // Remove the proc entry
}

// Register module entry and exit points
module_init(my_timer_init);
module_exit(my_timer_exit);
