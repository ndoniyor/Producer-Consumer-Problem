#include <time.h>
#include <wait.h>
#include <stdlib.h>
#include <unistd.h>	 

#define TRUE 1
#define FALSE 0
#define ERROR -1


int open_error_report(char* filename, int flags, int perm){
    int fd;
    if(perm>0) fd = open(filename, flags, perm);
    else fd = open(filename, flags);

    if (fd == ERROR){
        perror("open_error_report(): ");
        exit(1);
    }
    return fd;
}

void rand_delay(int mult){
    static int seed_check = FALSE; //static int to see if rand has been seeded

    if (seed_check==FALSE){         //seed random function
		srand(time(NULL));
		seed_check = TRUE;
    }

    int rand_num = mult * (rand() % 15);   //random number 1-15 with optional multiplier
                                       //for more control
    struct timespec delay_amt;
    delay_amt.tv_sec = 0;
    delay_amt.tv_nsec = 15 * rand_num;
    nanosleep(&delay_amt, NULL);
}