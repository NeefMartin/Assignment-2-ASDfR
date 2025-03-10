#include <pthread.h>
#include <time.h>
#include <sys/types.h>
#include <math.h>
#include <stdio.h>
#include <sched.h>
#include <evl/sched.h>
#include <evl/clock.h>
#include <evl/thread.h>


#define LOOPINTERVAL_MS 1

const int analysis_time_s = 10;             //Initialize analysistime
long jitter_array[(analysis_time_s*1000)];  //Create array for time analysis
int iteration = 0;                       //Initialize iteration

// Heavy computation function
void heavy_computation() {
    // 1. Large Matrix Multiplication (100x100)
    double A[10][10], B[10][10], C[10][10] = {0};
    for (int i = 0; i < 10; i++)
        for (int j = 0; j < 10; j++) {
            A[i][j] = i + j;
            B[i][j] = i - j;
        }
    for (int i = 0; i < 10; i++)
        for (int j = 0; j < 10; j++)
            for (int k = 0; k < 10; k++)
                C[i][j] += A[i][k] * B[k][j];

    // 2. Intense Floating-Point Computation
    double sum = 0.0;
    for (int i = 1; i < 100000; i++) {
        sum += sqrt(i) + exp(i % 20);
    }

}


void *start_routine(void *arg){
    
    struct timespec ts;             //Create timespec struct that holds time members for nanosleep clock.
    struct timespec start,end;      //Create timespec for loop timing anaysis
    long jitter_ns;                 //Initialize jitter variable
    int efd;                        //Initialize evl thread variable
    struct evl_sched_attrs attrs;   //Initialize attributes struct
    bool out_of_band_check;         //Init

    //Schedule thread on certain CPU core
    cpu_set_t cpu_set; //Define cpu_set 
    CPU_ZERO(&cpu_set); //Set everything to zero
    CPU_SET(1, &cpu_set); //Set core 1 assigned for Xenomai in RELbot
    sched_setaffinity(0,sizeof(cpu_set_t),&cpu_set);

    //attach thread to EVL
    efd=evl_attach_self("thread");

    // //Check if attachment was succesfull
    // if (efd < 0) {
    //     evl_printf("error attaching thread: %s\n", strerror(-efd));
    //     // handle error...
    // }

    //Set scheduling policy and priority
    attrs.sched_policy = SCHED_FIFO;
    attrs.sched_priority=90;
    evl_set_schedattr(efd, &attrs);

    //Check if caller runs out-of-band
    out_of_band_check=evl_is_inband();
    printf("Function out-of-band: %B\n", out_of_band_check);

    evl_read_clock(EVL_CLOCK_MONOTONIC,&ts); //sets the current time in ts.tv_nsec
    
    for (int i=0;i<=(analysis_time_s*1000);i++){
        //Gets the actual time for jitter calculation
        evl_read_clock(EVL_CLOCK_MONOTONIC,&start); 

        heavy_computation();

        //Increment timer
        ts.tv_nsec += LOOPINTERVAL_MS*1000000L; 
        
        //Manage tv_nsec overflow
        if (ts.tv_nsec >= 1000000000L) { 
            ts.tv_sec ++;
            ts.tv_nsec -= 1000000000L;
        }       

        //sleep until next time of ts
        int ret = evl_sleep_until(EVL_CLOCK_MONOTONIC,&ts); 
        if (ret !=0){
            perror("clock_nanosleep");
        }
        evl_read_clock(EVL_CLOCK_MONOTONIC,&end);

        //Calculates the jitter at runtime
        jitter_ns = (end.tv_sec - start.tv_sec) * 1000000000L + 
        (end.tv_nsec - start.tv_nsec);

        //Adds jitter to array for later analysis
        jitter_array[i] = jitter_ns - (LOOPINTERVAL_MS*1000000L);
        iteration=i;
    };
    evl_detach_self(); //Detach thread from EVL
    pthread_exit(NULL); //exit thread
    return NULL;
}; 

int main() {
    //start thread
    pthread_t thread;                                   //Create thread buffer pointer
    pthread_create(&thread,NULL,start_routine,NULL);    //Creates thread and stores ID to buffer pointed to by thread

    //thread exit
    pthread_join(thread,NULL);                          //wait for thread exit
    
    //start analysis
    long min_jitter=jitter_array[0],max_jitter=jitter_array[0],sum_jitter=0,avg_jitter,std_jitter;

    for (int i=0;i<(analysis_time_s*1000);i++){
        if (jitter_array[i]<min_jitter){
            min_jitter=jitter_array[i];
        };
        if (jitter_array[i]>max_jitter){
            max_jitter=jitter_array[i];
        };
        sum_jitter+=jitter_array[i];
    }
    avg_jitter=sum_jitter/(analysis_time_s*1000);
    std_jitter=sqrt(sum_jitter/(analysis_time_s*1000));
    
    //Print results
    printf("Jitter Analysis:\n");
    printf("Min Jitter: %ld ns\n", min_jitter);
    printf("Max Jitter: %ld ns\n", max_jitter);
    printf("Avg Jitter: %ld ns\n", avg_jitter);
    printf("Std Jitter: %ld ns\n", std_jitter);
    return 0;
}