#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h> // For kmalloc and kfree
#include <linux/list.h> // For linked list
#include <linux/mutex.h> // For mutexes
#include <linux/delay.h> // For msleep

MODULE_LICENSE("GPL");
MODULE_AUTHOR("cop4610t");
MODULE_DESCRIPTION("Example of kernel module proc file for elevator");

#define ENTRY_NAME "elevator_example"
#define PERMS 0644
#define PARENT NULL

static struct proc_dir_entry* elevator_entry;
static struct mutex elevator_mutex; // Mutex for thread safety

typedef struct passenger {
    int weight;
    int destination_floor;
    struct list_head list;
} Passenger;

typedef struct elevator {
    int current_floor;
    int total_weight;
    int passenger_count;
    struct list_head passengers;
} Elevator;

static Elevator elevator = {
    .current_floor = 0,
    .total_weight = 0,
    .passenger_count = 0,
    .passengers = LIST_HEAD_INIT(elevator.passengers)
};

static ssize_t elevator_read(struct file *file, char __user *ubuf, size_t count,  loff_t *ppos) {
    char buf[10000];
    int len = 0;

    mutex_lock(&elevator_mutex);
    len = sprintf(buf, "Elevator state: \n");
    len += sprintf(buf + len, "Current floor: %d\n", elevator.current_floor);
    len += sprintf(buf + len, "Current load: %d\n", elevator.total_weight);
    len += sprintf(buf + len, "Passenger count: %d\n", elevator.passenger_count);
    mutex_unlock(&elevator_mutex);

    return simple_read_from_buffer(ubuf, count, ppos, buf, len); // Safer buffer handling
}

static const struct proc_ops elevator_fops = {
    .proc_read = elevator_read,
};

static int __init elevator_init(void) {
    elevator_entry = proc_create(ENTRY_NAME, PERMS, PARENT, &elevator_fops);
    mutex_init(&elevator_mutex); // Initialize the mutex
    if (!elevator_entry) {
        return -ENOMEM;
    }
    return 0;
}

static void __exit elevator_exit(void) {
    proc_remove(elevator_entry);
    mutex_destroy(&elevator_mutex); // Cleanup the mutex
}

module_init(elevator_init);
module_exit(elevator_exit);
