#include <sys/sem.h>
#include<signal.h>
#include<unistd.h>
#include<fcntl.h>
#include "auxiliary_functions.h"

#define BUF_SIZE 1024

void down_semaphore(int semaphore_id)
{
    struct sembuf sem_op;

    /* wait on the semaphore, unless it's value is non-negative. */
    sem_op.sem_num = 0;
    sem_op.sem_op = -1;
    sem_op.sem_flg = 0;
    semop(semaphore_id, &sem_op, 1);
}

void up_semaphore(int semaphore_id)
{
    struct sembuf sem_op;

    /* signal the semaphore - increase its value by one. */
    sem_op.sem_num = 0;
    sem_op.sem_op = 1;
    sem_op.sem_flg = 0;
    semop(semaphore_id, &sem_op, 1);
}

void consumer_op(pid_t pid, int semaphore_id, ssize_t* bytes_copied, char* buffer, char* file)
{
  
    int fd;
    ssize_t nwrite, nread;

    fd = open(file, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (fd < 0) {
        perror("consumer:");
        return;
    }

	do
    {
      	random_delay();        /* Wait for producer to write to shared buffer */
        down_semaphore(semaphore_id); /* Lock the resource */ 
        nread = *bytes_copied;
        if(nread > 0) {
            nwrite = write(fd, buffer, nread);
        	/* printf("\nconsumer write: %d bytes", nwrite); */
        	kill(pid, SIGUSR2);    /* Send signal to main process */
        }
        *bytes_copied = 0;
      	up_semaphore(semaphore_id);
    }
	while (nread >= 0);
  
  	close(fd);
    kill(pid, SIGUSR2);  /* After the file is copied, send signal to main */
    random_delay(); random_delay();
    kill(pid, SIGUSR2);  /* After the file is copied, send signal to main */
}

void producer_op(pid_t pid, int semaphore_id, ssize_t* bytes_copied, char* buffer, char* file)
{
  
    int fd;
    ssize_t nread;

    fd = open(file, O_RDONLY);
    if (fd < 0) {
        perror("producer:");
        return;
    }

    do {
    	down_semaphore(semaphore_id);   /* Lock the shared resource */
        nread = 1;
        if(*bytes_copied == 0) {
            nread = read(fd, buffer, BUF_SIZE);
            if(nread > 0)
                *bytes_copied = nread;
            else
                *bytes_copied = -1;
            /* printf("\nproducer read: %d bytes", nread); */
        	kill(pid, SIGUSR1); /* Send signal to main process */
        }  
    	up_semaphore(semaphore_id);
		random_delay();     /* Wait for consumer to read from shared resource */
    }
    while(nread > 0);

    close(fd);

}