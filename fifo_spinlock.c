/*
FIFO版的spinlock
使用方法：ticketlock [numThread]
如果不指定numThread，預設數量為4個
*/


#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h> 
#include <stdatomic.h>
#include <assert.h>
#include <signal.h>

#define NONE "\033[m"
#define RED "\033[0;32;31m"
#define YELLOW "\033[1;33m"

struct ticketlock_t {
	volatile atomic_int next_ticket;	//一定要加上volatile，否則-O3會出錯
	//一定要加上volatile，否則-O3會出錯，這一個不用是atomic_int，因為執行時它已經在CS
	volatile int now_serving;			
};

struct timespec ts = {0, 100};
struct timespec nanoSleepTs = {0, 0};

long long* inCS;
long masterID = 0;   //masterID is the biggest one
int isMutex = 0;
struct ticketlock_t tlock= {0};

void per_5seconds(int signum) {
    static int run = 0;
    printf(YELLOW"run [%3d]  "NONE, run++);
    for (int i=0; i<= masterID; i++) {
        printf("%lld ", inCS[i]);
    }
    printf("\n");
    alarm(5);
}

/*void ticketLock_acquire(volatile atomic_int* next_ticket,volatile int* now_serving){
    int my_ticket;
    int expected;
     my_ticket = atomic_fetch_add(next_ticket, 1);
     while(!atomic_compare_exchange_weak(now_serving,&expected,expected)){
        expected=my_ticket;
     }
    // while(*now_serving != my_ticket)
      //  ;
}*/


void ticketLock_release(volatile int *now_serving) {
    ++*now_serving;
}

void thread(void *_name) {
    long name = (long)_name;
    int my_ticket;
    int expected;
    int s, j;
    cpu_set_t cpuset;
    pthread_t thread;

    thread = pthread_self();
    CPU_ZERO(&cpuset);
    CPU_SET((int)name, &cpuset);
    pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);

    while(1) {
		long name = (long)_name;
                my_ticket=atomic_fetch_add(&(tlock.next_ticket),1);
                while(!atomic_compare_exchange_weak(&(tlock.now_serving),&expected,expected)){
                        expected=my_ticket;
                        }
		//ticketLock_acquire(&(tlock.next_ticket), &(tlock.now_serving));
        //isMutex是用來計算到底有多少人在CS內
        isMutex++;
        if (isMutex != 1) fprintf(stderr, "mutual execution");  
        inCS[name]++;
        isMutex--;
	tlock.now_serving++;
		//remainder section
    }
}


int main(void) {
    
    int numCPU = sysconf( _SC_NPROCESSORS_ONLN );
    printf("num of hardware thread = %d\n", numCPU);
    masterID = numCPU-1;
	pthread_t* tid = (pthread_t*)malloc(sizeof(pthread_t) * numCPU);
    inCS = malloc(numCPU * sizeof(long long));
    alarm(5);
    signal(SIGALRM, per_5seconds);
    //create slave
    for (long i=0; i< numCPU; i++) {
        pthread_create(&tid[i],NULL,(void *) thread, (void*)i);
    }
    //create master
    

    for (int i=0; i< numCPU; i++)
	    pthread_join(tid[i],NULL);
	return (0);
}

/*
int main(int argc, char** argv) {
	struct ticketlock_t myTicketlock= {0};
	atomic_store(&num_thread_in_cs, 0);
	pthread_t* ptid;
    pthread_t w_id;
    int nThread = 4;
    if (argc == 2)
        sscanf(argv[1], "%d", &nThread);
    printf("建立 %d 個 reader threads\n", nThread);

    ptid = (pthread_t*)malloc(sizeof(pthread_t) * nThread);

    for (int i=0; i<nThread; i++) {
	    int ret = pthread_create(&ptid[i], NULL, (void *)thread, &myTicketlock);
        assert(ret==0);
    }

    for (int i=0; i<nThread; i++) {
	    pthread_join(ptid[i],NULL);
    }
    
}*/
