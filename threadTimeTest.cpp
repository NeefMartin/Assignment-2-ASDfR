#include <pthread.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#define LOOPINTERVAL_MS 1

const int analysis_time_s = 10;             //Initialize analysistime
long jitter_array[(analysis_time_s*1000)];  //Create array for time analysis
int iteration = 0;                       //Initialize iteration

// Heavy computation function
void heavy_computation() {
    // 1. Large Matrix Multiplication (100x100)
    double A[100][100], B[100][100], C[100][100] = {0};
    for (int i = 0; i < 100; i++)
        for (int j = 0; j < 100; j++) {
            A[i][j] = i + j;
            B[i][j] = i - j;
        }
    for (int i = 0; i < 100; i++)
        for (int j = 0; j < 100; j++)
            for (int k = 0; k < 100; k++)
                C[i][j] += A[i][k] * B[k][j];

    // 2. Intense Floating-Point Computation
    double sum = 0.0;
    for (int i = 1; i < 1000000; i++) {
        sum += sqrt(i) + exp(i % 20);
    }

}


void *start_routine(void *arg){
    
    struct timespec ts;             //Create timespec struct that holds time members for nanosleep clock.
    struct timespec start,end;    //Create timespec for loop timing anaysis
    long jitter_ns;                 //Initialize jitter variable

    clock_gettime(CLOCK_MONOTONIC,&ts); //sets the current time in ts.tv_nsec
    
    for (int i=0;i<=(analysis_time_s*1000);i++){
        //Gets the actual time for jitter calculation
        clock_gettime(CLOCK_MONOTONIC,&start); 

        heavy_computation();

        //Increment timer
        ts.tv_nsec += LOOPINTERVAL_MS*1000000L; 
        
        //Manage tv_nsec overflow
        if (ts.tv_nsec >= 1000000000L) { 
            ts.tv_sec ++;
            ts.tv_nsec -= 1000000000L;
        }       

        //sleep until next time of ts
        int ret = clock_nanosleep(CLOCK_MONOTONIC,TIMER_ABSTIME,&ts,NULL); 
        if (ret !=0){
            perror("clock_nanosleep");
        }
        clock_gettime(CLOCK_MONOTONIC,&end);

        //Calculates the jitter at runtime
        jitter_ns = (end.tv_sec - start.tv_sec) * 1000000000L + 
        (end.tv_nsec - start.tv_nsec);

        //Adds jitter to array for later analysis
        jitter_array[i] = jitter_ns - (LOOPINTERVAL_MS*1000000L);
        iteration=i;
    };
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
