#include <linux/time.h>
#include <cstdio>
#include <linux/ktime.h>


static ktime_t time;

int main (){

struct timespec64 now;
time = ktime_get_real_ts64();
printk("Timer: ", time.t) 

return 0;
}