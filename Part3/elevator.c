#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/kthread.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group 19");
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
    int num_passengers_type[MAX_PASSENGER_TYPES]; // Track number of passengers for each type
} Elevator;

static Elevator elevator = {
    .state = OFFLINE,
    .current_floor = 1,
    .total_weight = 0,
    .passenger_count = 0,
    .num_passengers_type = {0}, // Initialize array to all zeros
    .passengers = LIST_HEAD_INIT(elevator.passengers)
};

static Floor floors[MAX_FLOORS];

// Function prototypes
static void unload_passengers(void);
static void load_passengers(void);
static void move_up(void);
static void move_down(void);
static int start_elevator(void);
static int stop_elevator(void);
static int issue_request(int,int,int);
static bool should_stop(int);
static void decide_next_action(void);

extern int (*STUB_start_elevator)(void);
int start_elevator(void) {
    mutex_lock(&elevator_mutex);
    if (elevator.state != OFFLINE) {
        mutex_unlock(&elevator_mutex);
        pr_err("Elevator cannot be started. It is not in the OFFLINE state.\n");
        return -EINVAL; 
    }
    elevator.state = IDLE;
    pr_info("Elevator started successfully.\n");
    mutex_unlock(&elevator_mutex);
    return 0;
}

extern int (*STUB_stop_elevator)(void);
int stop_elevator(void) {
    Passenger *passenger, *temp;

    mutex_lock(&elevator_mutex);

    // Check if the elevator is already offline.
    if (elevator.state == OFFLINE) {
        mutex_unlock(&elevator_mutex);
        return 0; 
    }

    elevator.state = OFFLINE;

    mutex_unlock(&elevator_mutex);

    if (elevator_thread) {
        kthread_stop(elevator_thread);
        elevator_thread = NULL; 
    }
mutex_lock(&elevator_mutex);
    list_for_each_entry_safe(passenger, temp, &elevator.passengers, list) {
        list_del(&passenger->list); 
        kfree(passenger); 
    }
    mutex_unlock(&elevator_mutex);

    pr_info("Elevator stopped successfully.\n");

    return 0;
}

extern int (*STUB_issue_request)(int, int, int);
int issue_request(int start, int dest, int type) {
    if (start < 1 || start > MAX_FLOORS || dest < 1 || dest > MAX_FLOORS) {
        return -EINVAL;
    }


    // Allocate memory for the new passenger
    Passenger *new_passenger = kmalloc(sizeof(Passenger), GFP_KERNEL);
    if (!new_passenger) {
        printk("Cannot allocate memory for new passenger.\n");
        return -ENOMEM;
    }

    new_passenger->type = type;
    new_passenger->destination_floor = dest;

    // Assign weight based on passenger type
    switch(type) {
    case 0: // Part-time worker
	new_passenger->type ='P';
        new_passenger->weight = 1;
        break;
    case 1: // Lawyer
        new_passenger->type ='L';
        new_passenger->weight = 1.5;
        break;
    case 2: // Boss
        new_passenger->type ='B';
        new_passenger->weight = 2;
        break;
    case 3: // Visitor
        new_passenger->type ='V';
        new_passenger->weight = 0.5;
        break;
    default:
        printk("Invalid passenger type.\n");
        kfree(new_passenger);
        return -EINVAL;
    }
    mutex_lock(&floors[start - 1].floor_mutex);
    list_add_tail(&new_passenger->list, &floors[start - 1].passengers);
    floors[start - 1].num_passengers_waiting++;
    mutex_unlock(&floors[start - 1].floor_mutex);

    printk("New passenger request added: Start: %d, Dest: %d, Type: %c\n", start, dest, type);

    mutex_lock(&elevator_mutex);
    if (elevator.state == IDLE) {
        if (elevator.current_floor < start) {
            elevator.state = UP;
        } else if (elevator.current_floor > start) {
            elevator.state = DOWN;
        } else {
            elevator.state = LOADING;
        }
    }
    mutex_unlock(&elevator_mutex);
    return 0;
}
static int elevator_thread_function(void *data) {
    while (!kthread_should_stop()) {
        // Check the elevator's current state
        switch(elevator.state) {
            case LOADING:
                // Unload and load passengers
                unload_passengers();
                load_passengers();

                // Simulate loading/unloading time
                msleep(2000); // 2 seconds = 2000 milliseconds

                // Decide next action: Continue moving or stay idle if no passengers to service
                decide_next_action();
                break;

            case UP:
                move_up();
                msleep(2000); // Simulate time taken to move between floors
                break;

            case DOWN:
                move_down();
                msleep(2000); // Simulate time taken to move between floors
                break;

            case IDLE:
            case OFFLINE:
                // In IDLE or OFFLINE state, just wait a bit before checking again
                msleep(100); // Short sleep to prevent busy waiting
                break;
        }
    }
    return 0;
}

static void decide_next_action(void) {
    // Placeholder logic for deciding next action
    bool upDirectionHasPassengers = false;
    bool downDirectionHasPassengers = false;

    // Check for passengers above current floor needing to go up or down
    for (int i = elevator.current_floor; i < MAX_FLOORS; i++) {
        if (floors[i].num_passengers_waiting > 0) {
            upDirectionHasPassengers = true;
            break;
        }
    }

    // Check for passengers below current floor needing to go up or down
    for (int i = elevator.current_floor - 2; i >= 0; i--) { // i starts from current_floor - 2 because array indexing starts at 0
        if (floors[i].num_passengers_waiting > 0) {
            downDirectionHasPassengers = true;
            break;
        }
    }

    // Determine next state based on where passengers are waiting
    if (upDirectionHasPassengers) {
        elevator.state = UP;
    } else if (downDirectionHasPassengers) {
        elevator.state = DOWN;
    } else {
        elevator.state = IDLE; // No passengers waiting, go idle
    }
}

static int get_num_waiting(void) {
    int waiting = 0;

    for(int i = 0; i < MAX_FLOORS; i++) {
	mutex_lock(&floors[i].floor_mutex);
        waiting += floors[i].num_passengers_waiting;
	mutex_unlock(&floors[i].floor_mutex);
    }

    return waiting;
}

static void unload_passengers(void) {
    Passenger *passenger, *temp;

    mutex_lock(&elevator_mutex); // Lock before modifying shared resources
    list_for_each_entry_safe(passenger, temp, &elevator.passengers, list) {
        if (passenger->destination_floor == elevator.current_floor) {
            elevator.total_weight -= passenger->weight; // Update weight
            elevator.passenger_count--; // Update passenger count
            list_del(&passenger->list); // Remove passenger from list
            kfree(passenger); // Free the passenger structure
        }
    }
    mutex_unlock(&elevator_mutex); // Unlock after modifications
}

static void load_passengers(void) {
Passenger *passenger, *temp;
    Floor *current_floor = &floors[elevator.current_floor - 1];

    // Iterate through the passengers waiting on the current floor
    list_for_each_entry_safe(passenger, temp, &current_floor->passengers, list) {
        // Check if elevator can accommodate the passenger
        if (elevator.total_weight + passenger->weight <= MAX_WEIGHT && elevator.passenger_count < MAX_PASSENGERS) {
            mutex_lock(&elevator_mutex);
            current_floor->num_passengers_waiting--;
            list_del(&passenger->list);
            list_add_tail(&passenger->list, &elevator.passengers);
            elevator.total_weight += passenger->weight;
            elevator.passenger_count++;
            elevator.num_passengers_type[passenger->type - 'A']++; // Increment count for passenger type
            mutex_unlock(&elevator_mutex);
        } else {
            break; // Elevator is full or overweight
        }
    }
}
static void move_up(void) {
    elevator.current_floor++;
    if (elevator.current_floor == MAX_FLOORS) {
        elevator.state = DOWN; // Change direction when reaching the top floor
    } else {
        // Check if there are passengers to unload or load at the new current floor
        if (should_stop(elevator.current_floor)) {
            elevator.state = LOADING;
            unload_passengers();
        }
    }
}
static void move_down(void) {
    elevator.current_floor--;
    if (elevator.current_floor == 1) {
        elevator.state = UP; // Change direction when reaching the bottom floor
    } else {
        // Check if there are passengers to unload or load at the new current floor
        if (should_stop(elevator.current_floor)) {
            elevator.state = LOADING;
            unload_passengers();
        }
    }
}

static bool should_stop(int floor) {
    Floor *current_floor = &floors[floor - 1];
    Passenger *passenger;
    
    // Check if any passengers in the elevator need to get off at this floor
    list_for_each_entry(passenger, &elevator.passengers, list) {
        if (passenger->destination_floor == floor) {
            return true;
        }
    }
    
    // Check if there are passengers waiting at this floor
    if (current_floor->num_passengers_waiting > 0) {
        return true;
    }
    
    return false;
}
static ssize_t elevator_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos) {
    char *buf;
    ssize_t len = 0;
    Passenger *passenger;

    buf = kmalloc(4096, GFP_KERNEL); // Dynamically allocate memory
    if (!buf) {
        return -ENOMEM; // Memory allocation failed
    }

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
    len += sprintf(buf + len, "Current floor: %d\n", elevator.current_floor); // Include current floor
    len += sprintf(buf + len, "Current load: %d lbs\n", elevator.total_weight);
    len += sprintf(buf + len, "Elevator status: ");
    list_for_each_entry(passenger, &elevator.passengers, list) {
        len += sprintf(buf + len, "%c%d ", passenger->type, passenger->destination_floor);
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

    int waiting = get_num_waiting();

    len += sprintf(buf + len, "\n");
    len += sprintf(buf + len, "Number of passengers: %d\n", elevator.passenger_count);
    len += sprintf(buf + len, "Number of passengers waiting: %d\n", waiting);
    len += sprintf(buf + len, "Number of passengers serviced: %d\n", waiting);

    // Copy buffer to user space
    len = simple_read_from_buffer(ubuf, count, ppos, buf, len);
    kfree(buf); // Free dynamically allocated memory
    return len;
}
static const struct proc_ops elevator_fops = {
    .proc_read = elevator_read,
};

static int __init elevator_init(void) {
    STUB_start_elevator = start_elevator;
    STUB_issue_request = issue_request;
    STUB_stop_elevator = stop_elevator;
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
    STUB_start_elevator = NULL;
    STUB_issue_request = NULL;
    STUB_stop_elevator = NULL;

    kthread_stop(elevator_thread);
    proc_remove(elevator_entry);
    mutex_destroy(&elevator_mutex);
    for (int i = 0; i < MAX_FLOORS; ++i) {
        mutex_destroy(&floors[i].floor_mutex);
    }
}

module_init(elevator_init);
module_exit(elevator_exit);
