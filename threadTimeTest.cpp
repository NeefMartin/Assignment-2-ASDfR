#include <pthread.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#define LOOPINTERVAL_MS 1

const int analysis_time_s = 10;             //Initialize analysistime
long jitter_array[(analysis_time_s*1000)];  //Create array for time analysis
int iteration = 0;                       //Initialize iteration


void *start_routine(void *arg){
    
    struct timespec ts;             //Create timespec struct that holds time members for nanosleep clock.
    struct timespec start,end;    //Create timespec for loop timing anaysis
    long jitter_ns;                 //Initialize jitter variable

    clock_gettime(CLOCK_MONOTONIC,&ts); //sets the current time in ts.tv_nsec
    
    for (int i=0;i<=(analysis_time_s*1000);i++){
        //Gets the actual time for jitter calculation
        clock_gettime(CLOCK_MONOTONIC,&start); 

        
        //Increment timer
        ts.tv_nsec += LOOPINTERVAL_MS*1000000; 
        
        //Manage tv_nsec overflow
        if (ts.tv_nsec >= 1000000000) { 
            ts.tv_sec += 1;
            ts.tv_nsec -= 1000000000;
        }

        //sleep until next time of ts
        clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&ts,NULL); 
        clock_gettime(CLOCK_MONOTONIC,&end);

        //Calculates the jitter at runtime
        jitter_ns = (end.tv_sec - start.tv_sec) * 1000000000L + 
        (end.tv_nsec - start.tv_nsec);

        //Adds jitter to array for later analysis
        jitter_array[i] = jitter_ns - (LOOPINTERVAL_MS*1000000L);
        iteration=i;
    };
    printf("Exiting thread after %ld iterations\n",iteration);
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
