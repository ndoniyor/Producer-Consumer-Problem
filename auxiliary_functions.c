#include<time.h>

void random_delay()
{
    static int initialized = 0;
    int rand_num;
    struct timespec delay;

    if (!initialized) {         //seed random function
		srand(time(NULL));
		initialized = 1;
    }

    rand_num = rand() % 10;
    delay.tv_sec = 0;
    delay.tv_nsec = 10 * rand_num;
    nanosleep(&delay, NULL);
}