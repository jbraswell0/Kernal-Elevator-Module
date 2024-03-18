#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/kthread.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Elevator kernel module");

#define ENTRY_NAME "elevator"
#define PERMS 0644
#define PARENT NULL

static struct proc_dir_entry* elevator_entry;
static struct mutex elevator_mutex; 
static struct task_struct *elevator_thread;

#define MAX_FLOORS 5
#define MAX_PASSENGER_TYPES 4
#define MAX_PASSENGERS 5
#define MAX_WEIGHT 7

typedef enum {OFFLINE, IDLE, LOADING, UP, DOWN} ElevatorState;

typedef struct passenger {
    char type; // P, L, B, V
    int destination_floor;
    int weight;
    struct list_head list;
} Passenger;

typedef struct floor {
    int floor_number;
    int num_passengers_waiting;
    struct list_head passengers;
    struct mutex floor_mutex;
} Floor;

typedef struct elevator {
    ElevatorState state;
    int current_floor;
    int total_weight;
    int passenger_count;
    struct list_head passengers;
} Elevator;

static Elevator elevator = {
    .state = OFFLINE,
    .current_floor = 1,
    .total_weight = 0,
    .passenger_count = 0,
    .passengers = LIST_HEAD_INIT(elevator.passengers)
};

static Floor floors[MAX_FLOORS];

// Function prototypes
static void unload_passengers(void);
static void load_passengers(void);
static void move_up(void);
static void move_down(void);

static int elevator_thread_function(void *data) {
    while (!kthread_should_stop()) {
        // Elevator movement logic
        if (elevator.state == LOADING) {
            // Unload passengers
            unload_passengers();
            // Load passengers
            load_passengers();
            msleep(1000); // Wait 1 second for loading/unloading
        } else if (elevator.state == UP) {
            move_up();
            msleep(2000); // Wait 2 seconds between floors
        } else if (elevator.state == DOWN) {
            move_down();
            msleep(2000); // Wait 2 seconds between floors
        } else {
            msleep(100); // Idle state, wait for next action
        }
    }
    return 0;
}

static void unload_passengers(void) {
    Passenger *passenger, *temp;
    list_for_each_entry_safe(passenger, temp, &elevator.passengers, list) {
        if (passenger->destination_floor == elevator.current_floor) {
            mutex_lock(&elevator_mutex);
            elevator.total_weight -= passenger->weight;
            elevator.passenger_count--;
            list_del(&passenger->list);
            kfree(passenger);
            mutex_unlock(&elevator_mutex);
        }
    }
}

static void load_passengers(void) {
    Floor *floor = &floors[elevator.current_floor - 1];
    Passenger *passenger, *temp;
    list_for_each_entry_safe(passenger, temp, &floor->passengers, list) {
        if (elevator.total_weight + passenger->weight <= MAX_WEIGHT && elevator.passenger_count < MAX_PASSENGERS) {
            mutex_lock(&elevator_mutex);
            floor->num_passengers_waiting--;
            list_del(&passenger->list);
            list_add_tail(&passenger->list, &elevator.passengers);
            elevator.total_weight += passenger->weight;
            elevator.passenger_count++;
            mutex_unlock(&elevator_mutex);
        } else {
            break; // Elevator is full
        }
    }
}

static void move_up(void) {
    elevator.current_floor++;
    if (elevator.current_floor == MAX_FLOORS) {
        elevator.state = DOWN; // Change direction when reaching the top floor
    }
}

static void move_down(void) {
    elevator.current_floor--;
    if (elevator.current_floor == 1) {
        elevator.state = UP; // Change direction when reaching the bottom floor
    }
}

static ssize_t elevator_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos) {
    char buf[4096];
    int len = 0;
    Passenger *passenger;

    mutex_lock(&elevator_mutex);
    len += sprintf(buf + len, "Elevator state: ");
    switch (elevator.state) {
        case OFFLINE:
            len += sprintf(buf + len, "OFFLINE\n");
            break;
        case IDLE:
            len += sprintf(buf + len, "IDLE\n");
            break;
        case LOADING:
            len += sprintf(buf + len, "LOADING\n");
            break;
        case UP:
            len += sprintf(buf + len, "UP\n");
            break;
        case DOWN:
            len += sprintf(buf + len, "DOWN\n");
            break;
    }
    len += sprintf(buf + len, "Current floor: %d\n", elevator.current_floor);
    len += sprintf(buf + len, "Current load: %d lbs\n", elevator.total_weight);
    len += sprintf(buf + len, "Passenger count: %d\n", elevator.passenger_count);
    list_for_each_entry(passenger, &elevator.passengers, list) {
        len += sprintf(buf + len, "Passenger type: %c, Destination floor: %d\n", passenger->type, passenger->destination_floor);
    }
    len += sprintf(buf + len, "\n");

    // Print floors information
    for (int i = 0; i < MAX_FLOORS; i++) {
        mutex_lock(&floors[i].floor_mutex);
        len += sprintf(buf + len, "[%c] Floor %d: %d ", (elevator.current_floor == floors[i].floor_number) ? '*' : ' ', floors[i].floor_number, floors[i].num_passengers_waiting);
        list_for_each_entry(passenger, &floors[i].passengers, list) {
            len += sprintf(buf + len, "%c%d ", passenger->type, passenger->destination_floor);
        }
        len += sprintf(buf + len, "\n");
        mutex_unlock(&floors[i].floor_mutex);
    }

    mutex_unlock(&elevator_mutex);

    return simple_read_from_buffer(ubuf, count, ppos, buf, len);
}

static const struct proc_ops elevator_fops = {
    .proc_read = elevator_read,
};

static int __init elevator_init(void) {
    elevator_entry = proc_create(ENTRY_NAME, PERMS, PARENT, &elevator_fops);
    mutex_init(&elevator_mutex);
    if (!elevator_entry) {
        return -ENOMEM;
    }

    // Initialize floors
    for (int i = 0; i < MAX_FLOORS; ++i) {
        floors[i].floor_number = i + 1;
        floors[i].num_passengers_waiting = 0;
        mutex_init(&floors[i].floor_mutex);
        INIT_LIST_HEAD(&floors[i].passengers);
    }

    // Create kthread for elevator movement
    elevator_thread = kthread_create(elevator_thread_function, NULL, "elevator_thread");
    if (IS_ERR(elevator_thread)) {
        pr_err("Failed to create elevator thread\n");
        return PTR_ERR(elevator_thread);
    }
    wake_up_process(elevator_thread);

    return 0;
}

static void __exit elevator_exit(void) {
    kthread_stop(elevator_thread);
    proc_remove(elevator_entry);
    mutex_destroy(&elevator_mutex);
    for (int i = 0; i < MAX_FLOORS; ++i) {
        mutex_destroy(&floors[i].floor_mutex);
    }
}

module_init(elevator_init);
module_exit(elevator_exit);
