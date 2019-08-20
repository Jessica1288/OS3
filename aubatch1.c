/* 
 * COMP7500/7506
 * Project 3: AUbatch - A Batch Scheduling System
 *
 * Xiao Qin
 * Department of Computer Science and Software Engineering
 * Auburn University
 * Feb. 20, 2018. Version 1.1
 *
 * This sample source code demonstrates the development of 
 * a batch-job scheduler using pthread.
 *
 * Compilation Instruction: 
 * gcc pthread_sample.c -o pthread_sample -lpthread
 *
 * Learning Objecties:
 * 1. To compile and run a program powered by the pthread library
 * 2. To create two concurrent threads: a scheduling thread and a dispatching thread 
 * 3. To execute jobs in the AUbatch system by the dispatching thread
 * 4. To synchronize the two concurrent threads using condition variables
 *
 * How to run aubatch_sample?
 * 1. You need to compile another sample code: process.c
 * 2. The "process" program (see process.c) takes two input arguments
 * from the commandline
 * 3. In aubtach: type ./process 5 10 to submit program "process" as a job.
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <math.h>

typedef unsigned int u_int; 

//#define LOW_ARRIVAL_RATE /* Long arrivel-time interval */
#define LOW_SERVICE_RATE   /* Long service time */

/* 
 * Static commands are submitted to the job queue.
 * When comment out the following macro, job are submitted by users.
 */
//#define STATIC_COMMAND 

#define CMD_BUF_SIZE 10 /* The size of the command queueu */
#define NUM_OF_CMD   5  /* The number of submitted jobs   */
#define MAX_CMD_LEN  512 /* The longest commandline length */

/* 
 * When a job is submitted, the job must be compiled before it
 * is running by the executor thread (see also executor()).
 */

/* Error Code */
#define EINVAL       1
#define E2BIG        2

#define MAXMENUARGS  6 
#define MAXCMDLINE   64 


void *commandline( void *ptr ); /* To simulate job submissions and scheduling */
void *executor( void *ptr );    /* To simulate job execution */

void menu_execute(char *line, int isargs); 
int cmd_run(int nargs, char **args); 
int cmd_quit(int nargs, char **args); 
int cmd_list(int nargs, char **args);
void showmenu(const char *name, const char *x[]);
int cmd_helpmenu(int n, char **a);
int cmd_dispatch(char *cmd);
int cmd_test(int nargs, char **args);

int policy_fcfs(int nargs, char **args);
int policy_sjf(int nargs, char **args);
int policy_priority(int nargs, char **args);


pthread_mutex_t cmd_queue_lock;  /* Lock for critical sections */
pthread_cond_t cmd_buf_not_full; /* Condition variable for buf_not_full */
pthread_cond_t cmd_buf_not_empty; /* Condition variable for buf_not_empty */

/* Global shared variables */
u_int buf_head;
u_int buf_tail;
u_int count;
u_int policy = 0;// default is fcfs - 0, sjf - 1, pri - 2;
u_int total = 0;
u_int test = 0;

typedef struct 
{
   char job_name[10];
   int burstTime;
   int priority;
   time_t arrival_time_h;
   time_t arrival_time_m;
   time_t arrival_time_s;
   double arrival_time;
   double start_time;
   double fin_time;
   float CPU_Time;
   float wait_Time;
   float turnaround_Time;

}new_cmd;

new_cmd job_queue[CMD_BUF_SIZE], cmd_buffer[CMD_BUF_SIZE], temp;

/*
 *  Command table.
 */
static struct {
    const char *name;
    int (*func)(int nargs, char **args);
} cmdtable[] = {
    /* commands: single command must end with \n */
    {"test",  cmd_test},
    { "?\n",    cmd_helpmenu },
    { "h\n",    cmd_helpmenu },
    { "help\n", cmd_helpmenu },
    { "r",      cmd_run },
    { "run",    cmd_run },
    { "q\n",    cmd_quit },
    { "quit\n", cmd_quit },
    {"list\n",  cmd_list },
    {"l\n",     cmd_list },
    {"FCFS\n",  policy_fcfs},
    {"fcfs\n",  policy_fcfs},
    {"sjf\n",   policy_sjf},
    {"SJF\n",   policy_sjf},
    {"priority\n",policy_priority},
    {"PRIORITY\n",policy_priority},
    {"pri\n",policy_priority},
    {"PRI\n",policy_priority},
    
    /* Please add more operations below. */
    {NULL, NULL}
};

static const char *helpmenu[] = {
    "[run] <job> <time> <priority>       ",
    "[quit] Exit AUbatch                 ",
    "[help] Print help menu              ",
    "[fcfs] change the scheduling policy to fcfs",
    "[sjf] change the scheduling policy to SJF",
    "[priority] change the scheduling policy to PRIORITY",
    "[list] display the jobs",
        /* Please add more menu options below */
    NULL
};

// int cmd_test_run(int nargs, char **args) {
//     pthread_mutex_lock(&cmd_queue_lock);
//     int waiting = 0;
//     int i; 
//     time_t rawtime;
//     struct tm *info;
//     time(&rawtime);
//     info = localtime(&rawtime);

//     char *path = args[1];
//     char *policy_name = args[2];
//     size_t num_of_job = atoi(args[3]);

//     double cpu_time_sec_min = atoi(args[4]);
//     double cpu_time_sce_max = atoi(args[5]);

//     strcpy(job_queue[buf_head].job_name, args[1]);
//     job_queue[buf_head].burstTime = fmod(rand(),((cpu_time_sec_min) + (cpu_time_sce_max)));
//     job_queue[buf_head].priority = rand() % num_of_job;

//     job_queue[buf_head].arrival_time_h= info->tm_hour;
//     job_queue[buf_head].arrival_time_m= info->tm_min;
//     job_queue[buf_head].arrival_time_s= info->tm_sec;
//     job_queue[buf_head].arrival_time = (info->tm_hour)*60*60 + (info->tm_min)* 60 + (info->tm_sec);
//     for(i = buf_tail; i<buf_head; i++){
//         waiting += job_queue[i].burstTime;
//     }
//     // buf_head++;
//     // count++;

//     printf("Job %s was submitted.\n", args[1]);
//     printf("Total number of jobs in the queue : %d\n", count);
//     printf("Expected waiting time : %d\n", waiting); 
//     switch(policy){
//         case 0 : printf("Scheudling Policy : FCFS\n"); break;
//         case 1 : printf("Scheudling Policy : SJF\n"); break;
//         case 2 : printf("Scheudling Policy : Priority\n"); break;
//     }
//     pthread_mutex_unlock(&cmd_queue_lock);
//     return 0; /* if succeed */
// }

int cmd_test(int nargs, char **args){
    int j;
    pthread_mutex_lock(&cmd_queue_lock);
    new_cmd* test_value;
    test = 1;
    if(nargs < 5 ){
        printf("test <benchmark> <policy> <number of jobs> <min_cpu_time> <max_cpu_time>\n");
    }
    char *path = args[1];
    char *policy_name = args[2];
    size_t num_of_job = atoi(args[3]);

    double cpu_time_sec_min = atoi(args[4]);
    double cpu_time_sce_max = atoi(args[5]);

    for(int i = 0; policy_name[i]; i++){
        policy_name[i] = tolower(policy_name[i]);
    }
    printf("Test\n");
    printf("The benchmark name is: %s\n", path);
    printf("Policy is:%s\n", policy_name);
    printf("Number of job: %zu\n", num_of_job);
    printf("MIN_CPU time: %3.3f\n", cpu_time_sec_min);
    printf("MAX_CPU time: %3.3f\n", cpu_time_sce_max);
    
    char p0[] = "fcfs";
    char p1[] = "sjf";
    char p2[] = "priority";

    total = 0;

    for(j = 0; j < num_of_job; j++){
        pthread_mutex_unlock(&cmd_queue_lock);
        while(count == CMD_BUF_SIZE){
            pthread_cond_wait(&cmd_buf_not_full,&cmd_queue_lock);
        }
        count++;
        // cmd_test_run(nargs, args);
        strcpy(job_queue[buf_head].job_name, path);
        job_queue[buf_head].burstTime = fmod(rand(),((cpu_time_sec_min) + (cpu_time_sce_max)));
        job_queue[buf_head].priority = rand() % num_of_job;
        job_queue[buf_head].arrival_time = time(NULL);
        buf_head++;
        if(buf_head == CMD_BUF_SIZE){
            buf_head = 0;
        }
        
    }
    cmd_list(nargs, args);
    pthread_mutex_lock(&cmd_queue_lock);
    if(strcmp(policy_name, p0) == 0){//fcfs
        policy_fcfs(nargs, args);

    }
    if(strcmp(policy_name, p1) == 0){//sjf
        policy_sjf(nargs, args);       
    }
    if(strcmp(policy_name, p2) == 0){//priority
        policy_priority(nargs, args);
    }
    sleep(2);

    pthread_mutex_unlock(&cmd_queue_lock);
    cmd_quit(nargs, args);
    // double arrival_rate = 1.0;
    // buf_head++;
    // if(buf_head == CMD_BUF_SIZE){
    //     buf_head = 0;
    // }
    // cmd_quit(nargs, args);
    // sleep(1
    // return 0;
    // pthread_mutex_unlock(&cmd_queue_lock);
    return 0;
}
/*
 * The quit command.
 */
int cmd_quit(int nargs, char **args) {
    int i;
    float avg_turnaround = 0;
    float avg_cpu = 0;
    float avg_wait = 0;

    for(i=0; i<total; i++){
        avg_cpu += cmd_buffer[i].CPU_Time;
        avg_wait += cmd_buffer[i].wait_Time;
        avg_turnaround +=cmd_buffer[i].turnaround_Time;
    }
    avg_cpu = avg_cpu / total;
    avg_wait = avg_wait / total;
    avg_turnaround = avg_turnaround / total;

    printf("Total number of job submitted %d\n", total);
    printf("Average turnaround time :   %3.3f seconds\n", avg_turnaround);
    printf("Average CPU time :          %3.3f seconds\n", avg_cpu);
    printf("Average waiting time :      %3.3f seconds\n", avg_wait);  
    printf("Throughput :                %3.3f seconds\n", 1/avg_turnaround);  

    if(test == 1){
        test = 0;
        return 0;
    }
    else{
        exit(0);
    }
    
}


//sort the array using insertion sort algorithm

int policy_fcfs(int nargs, char **args){ 
    pthread_mutex_lock(&cmd_queue_lock);
    policy = 0;
    int i, j, key;
    for(i = buf_tail + 1 ; i < count; i++){ 
        for(j = i+1 ; j < count; j++){
            if((job_queue[i].arrival_time_h > job_queue[j].arrival_time_h) ||
                (job_queue[i].arrival_time_h == job_queue[j].arrival_time_h && 
                job_queue[i].arrival_time_m > job_queue[j].arrival_time_m )||
                (job_queue[i].arrival_time_h == job_queue[j].arrival_time_h &&
                job_queue[i].arrival_time_m == job_queue[j].arrival_time_m &&
                job_queue[i].arrival_time_s > job_queue[j].arrival_time_s)){
                    temp = job_queue[i];
                    job_queue[i] = job_queue[j];
                    job_queue[j] = temp;
            }
        }
    }
    pthread_mutex_unlock(&cmd_queue_lock);
    printf("Scheduling policy is switched to FCFS\n");
    if(test ==1)
    {
        printf("All the %d waiting jobs have been rescheduled.\n", total);
    }
    else{
        printf("All the %d waiting jobs have been rescheduled.\n", count);
    }
    
    return 0;
}

int policy_sjf(int nargs, char **args){ 
    pthread_mutex_lock(&cmd_queue_lock);
    policy = 1;
    int i, j;
    for(i = buf_tail + 1 ; i < count; i++){
        for(j = i+1 ; j < count; j++){
            if(job_queue[i].burstTime > job_queue[j].burstTime){
                temp = job_queue[i];
                job_queue[i] = job_queue[j];
                job_queue[j] = temp;
            }
        }
    }
    pthread_mutex_unlock(&cmd_queue_lock);
    printf("Scheduling policy is switched to SJF\n");
        if(test ==1)
    {
        printf("All the %d waiting jobs have been rescheduled.\n", total);
    }
    else{
        printf("All the %d waiting jobs have been rescheduled.\n", count);
    }
    return 0;
}

int policy_priority(int nargs, char **args){ // 1 is the highest priority, high(1) ---- low (10)
    pthread_mutex_lock(&cmd_queue_lock);
    policy = 2;
    int i, j;
    for(i = buf_tail + 1 ; i < count; i++){ 
        for(j = i+1 ; j < count; j++){
            if(job_queue[i].priority > job_queue[j].priority){
                temp = job_queue[i];
                job_queue[i] = job_queue[j];
                job_queue[j] = temp;
            }
        }
    }
    pthread_mutex_unlock(&cmd_queue_lock);
    printf("Scheduling policy is switched to Priority\n");
        if(test ==1)
    {
        printf("All the %d waiting jobs have been rescheduled.\n", total);
    }
    else{
        printf("All the %d waiting jobs have been rescheduled.\n", count);
    }
    return 0;
}

int cmd_list(int nargs, char **args){
    pthread_mutex_lock(&cmd_queue_lock);
    int i;
    struct tm *info;
    printf("Total number of jobs in the queue : %d\n", count);
    switch(policy){
        case 0 : printf("Scheudling policy : FCFS\n"); break;
        case 1 : printf("Scheudling Policy : SJF\n"); break;
        case 2 : printf("Scheudling Policy : Priority\n"); break;
    }
    printf("Name \t CPU_Time \t Pri \t Arrival_time \t Progress\n");


    for(i = buf_tail; i<count; i++){
        printf("%s \t", job_queue[i].job_name);
        printf("%d  \t\t", job_queue[i].burstTime);
        printf("%d   \t", job_queue[i].priority);
        // printf("%2ld:%02ld:%02ld \t",job_queue[i].arrival_time_h,job_queue[i].arrival_time_m, job_queue[i].arrival_time_s);
        // printf("%2d:%02d:%02d \n", info->tm_hour, info->tm_min, info->tm_sec);

        if(i == buf_tail)printf("Run");
        printf("\n");
    }
    pthread_mutex_unlock(&cmd_queue_lock);
    return 0;
}


/*
 * The run command - submit a job.
 */
int cmd_run(int nargs, char **args) {
    pthread_mutex_lock(&cmd_queue_lock);
    int waiting = 0;
    int i; 
    time_t rawtime;
    struct tm *info;
    time(&rawtime);
    info = localtime(&rawtime);

    // printf("%2d:%02d:%02d\n",(info ->tm_hour), info->tm_min, info->tm_sec);

    if (nargs != 4) {
        printf("Usage: run <job> <time> <priority>\n");
        return EINVAL;
    }
    // pthread_mutex_lock(&cmd_queue_lock);
    // while(count == CMD_BUF_SIZE){
    //     pthread_cond_wait(&cmd_buf_not_full,&cmd_queue_lock);
    // }

    strcpy(job_queue[buf_head].job_name, args[1]);
    job_queue[buf_head].burstTime = atoi(args[2]);
    job_queue[buf_head].priority = atoi(args[3]);
    job_queue[buf_head].arrival_time_h= info->tm_hour;
    job_queue[buf_head].arrival_time_m= info->tm_min;
    job_queue[buf_head].arrival_time_s= info->tm_sec;
    job_queue[buf_head].arrival_time = (info->tm_hour)*60*60 + (info->tm_min)* 60 + (info->tm_sec);
    for(i = buf_tail; i<buf_head; i++){
        waiting += job_queue[i].burstTime;
    }
    buf_head++;
    count++;

    printf("Job %s was submitted.\n", args[1]);
    printf("Total number of jobs in the queue : %d\n", count);
    printf("Expected waiting time : %d\n", waiting); 
    switch(policy){
        case 0 : printf("Scheudling Policy : FCFS\n"); break;
        case 1 : printf("Scheudling Policy : SJF\n"); break;
        case 2 : printf("Scheudling Policy : Priority\n"); break;
    }
    pthread_mutex_unlock(&cmd_queue_lock);
    return 0; /* if succeed */
}

int cmd_helpmenu(int n, char **a)
{
    pthread_mutex_lock(&cmd_queue_lock);
    (void)n;
    (void)a;

    showmenu("AUbatch help menu", helpmenu);
    return 0;
    pthread_mutex_unlock(&cmd_queue_lock);
}

/*
 * Display menu information
 */
void showmenu(const char *name, const char *x[])
{
    int ct, half, i;

    printf("\n");
    printf("%s\n", name);
    
    for (i=ct=0; x[i]; i++) {
        ct++;
    }
    half = (ct+1)/2;

    for (i=0; i<half; i++) {
        printf("    %-36s", x[i]);
        if (i+half < ct) {
            printf("%s", x[i+half]);
        }
        printf("\n");
    }

    printf("\n");
}


int main() {
    pthread_t command_thread, executor_thread; /* Two concurrent threads */

    /* Initilize count, two buffer pionters */
    count = 0; 
    buf_head = 0;  
    buf_tail = 0; 
    char *buffer;
    size_t bufsize = 64;
    new_cmd *input = NULL;

    /* Create two independent threads:command and executors */
    pthread_create(&command_thread, NULL, commandline, NULL);
    pthread_create(&executor_thread, NULL, executor, NULL);

    printf("Welcome to Jaewon's batch job scheduler Version 1.0 \n");
    printf("Type 'help' to find more about AUbatch commands.\n");
    while(1){
        // pthread_create(&command_thread, NULL, commandline, (void* )input);
        printf("> [? for menu]: ");
        buffer = (char*) malloc(bufsize * sizeof(char));
        getline(&buffer, &bufsize, stdin);
        cmd_dispatch(buffer);  
    }

    if(buffer == NULL){
        perror("Unable to malloc buffer");
        exit(1);
    }

    /* Initialize the lock the two condition variables */
    pthread_mutex_init(&cmd_queue_lock, NULL);
    pthread_cond_init(&cmd_buf_not_full, NULL);
    pthread_cond_init(&cmd_buf_not_empty, NULL);
     
    /* Wait till threads are complete before main continues. Unless we  */
    /* wait we run the risk of executing an exit which will terminate   */
    /* the process and all threads before the threads have completed.   */
    pthread_join(command_thread, NULL);
    pthread_join(executor_thread, NULL); 

    // pthread_attr_destroy(&attr);

    // printf("command_thread returns: %d\n",iret1);
    // printf("executor_thread returns: %d\n",iret1);
    exit(0);
}



int cmd_dispatch(char *cmd)
{
    time_t beforesecs, aftersecs, secs;
    u_int32_t beforensecs, afternsecs, nsecs;
    char *args[MAXMENUARGS];
    int nargs=0;
    char *word;
    char *context;
    int i, result;

    for (word = strtok_r(cmd, " ", &context);
         word != NULL;
         word = strtok_r(NULL, " ", &context)) {
        if (nargs >= MAXMENUARGS) {
            printf("Command line has too many words\n");
            return E2BIG;
        }
        args[nargs++] = word;
    }

    if (nargs==0) {
        return 0;
    }

    for (i=0; cmdtable[i].name; i++) {
        if (*cmdtable[i].name && !strcmp(args[0], cmdtable[i].name)) {
            assert(cmdtable[i].func!=NULL);
            
            /*Qin: Call function through the cmd_table */
            result = cmdtable[i].func(nargs, args);
            return result;
        }
    }
    // pthread_create(&command_thread, NULL, commandline, (void*) message1);
    printf("%s: Command not found\n", args[0]);
    return EINVAL;
}

/* 
 * This function simulates a terminal where users may 
 * submit jobs into a batch processing queue.
 * Note: The input parameter (i.e., *ptr) is optional. 
 * If you intend to create a thread from a function 
 * with input parameters, please follow this example.
 */
void *commandline(void *ptr) {
    u_int i;
    char num_str[8];
    size_t command_size;
    time_t arrival;

    new_cmd *newcommand = (new_cmd*)ptr;

    // char *buffer;
    // size_t bufsize = 64;
    // buffer = (char*) malloc(bufsize * sizeof(char));
    // printf("Welcome to Jaewon's batch job scheduler Version 1.0 \n");
    // printf("Type 'help' to find more about AUbatch commands.\n");
    // while(1){
    pthread_mutex_lock(&cmd_queue_lock);

    while(count == CMD_BUF_SIZE){
        pthread_cond_wait(&cmd_buf_not_full, &cmd_queue_lock);
    }
    pthread_mutex_lock(&cmd_queue_lock);
    // cmd_buffer[buf_head] = buffer;
    cmd_buffer[buf_head] = job_queue[buf_head];

    count++;
    buf_head++;
    if(buf_head == CMD_BUF_SIZE)
        buf_head = 0;

    pthread_cond_signal(&cmd_buf_not_empty);
    pthread_mutex_unlock(&cmd_queue_lock);

    sleep(2);
}

void *executor(void *ptr) {
    // char *message;
    // u_int i;
    char *exe[5];
    pid_t pid;
    time_t start;
    time_t fin; 
    struct tm *st_info, *fin_info;
    time(&start);
    st_info = localtime(&start);
    double start_time = (st_info->tm_hour)*60*60 +(st_info->tm_min)*60 + (st_info->tm_sec);
    double fin_time = 0;


    while(1){
        // pthread_create(&executor_thread, NULL, executor, (void*) message2);
        /* lock and unlock for the shared process queue */
        pthread_mutex_lock(&cmd_queue_lock);
        // printf("In executor: count = %d\n", count);
        
        while (count == 0) {
            pthread_cond_wait(&cmd_buf_not_empty, &cmd_queue_lock);
        }

        if(test == 1){
        // printf("In executor: cmd_buffer[%d] = %s\n", buf_tail, cmd_buffer[buf_tail]); 
        exe[0] = job_queue[buf_tail].job_name;
        exe[1] = job_queue[buf_tail].burstTime;
        exe[2] = NULL;
        job_queue[buf_tail].start_time = time(NULL);
        
        // pthread_mutex_unlock(&cmd_queue_lock);
        switch((pid=fork())){
            case -1:
                perror("fork");
                break;
            case 0:     //child process replace
                execv(exe[0],exe);
                // puts("Error during execv()");
                exit(-1);
            default:  //parent process print 
                // wait(NULL);
                // pthread_mutex_lock(&cmd_queue_lock);
                // job_queue[buf_tail].fin_time = time(&fin);
                // job_queue[buf_tail].CPU_Time = difftime(job_queue[buf_tail].fin_time, job_queue[buf_tail].start_time);
                // job_queue[buf_tail].wait_Time = difftime(job_queue[buf_tail].start_time, job_queue[buf_tail].arrival_time);
                // job_queue[buf_tail].turnaround_Time = difftime(job_queue[buf_tail].fin_time, job_queue[buf_tail].arrival_time);
                // memcpy(cmd_buffer, job_queue, strlen(job_queue)+1);
                // total++;
                break;
            }
            // test = 0;
        }
        
        time(&fin);
        fin_info = localtime(&fin);
        fin_time = (fin_info->tm_hour)*60*60 +(fin_info->tm_min)*60 + (fin_info->tm_sec);
        wait(NULL);
        pthread_mutex_lock(&cmd_queue_lock);
        job_queue[buf_tail].fin_time = fin_time;
        job_queue[buf_tail].CPU_Time = fabs(job_queue[buf_tail].fin_time - job_queue[buf_tail].start_time);
        job_queue[buf_tail].wait_Time = fabs(job_queue[buf_tail].start_time - job_queue[buf_tail].arrival_time);
        job_queue[buf_tail].turnaround_Time = fabs (job_queue[buf_tail].fin_time - job_queue[buf_tail].arrival_time);            
        count--;
        // printf("fin_time %3.3f\n", job_queue[buf_tail].fin_time);
        // printf("cpu_time %3.3f\n", job_queue[buf_tail].CPU_Time);
        // printf("wait_time %3.3f\n", job_queue[buf_tail].wait_Time);

        // memcpy(cmd_buffer, job_queue, strlen(job_queue)+1);
        
        strcpy(cmd_buffer[buf_tail].job_name , job_queue[buf_tail].job_name);
        cmd_buffer[buf_tail].burstTime = job_queue[buf_tail].burstTime;
        cmd_buffer[buf_tail].priority = job_queue[buf_tail].priority;
        cmd_buffer[buf_tail].arrival_time = job_queue[buf_tail].arrival_time;
        cmd_buffer[buf_tail].start_time = job_queue[buf_tail].start_time;
        cmd_buffer[buf_tail].fin_time = job_queue[buf_tail].fin_time;
        cmd_buffer[buf_tail].CPU_Time = job_queue[buf_tail].CPU_Time;
        cmd_buffer[buf_tail].wait_Time = job_queue[buf_tail].wait_Time;
        cmd_buffer[buf_tail].turnaround_Time = job_queue[buf_tail].turnaround_Time;


        total++;
        buf_tail++;

        // printf("cmd_buffer :%s\n", cmd_buffer[buf_tail].job_name);
        
        // printf("total : %d\n", total);
        // printf("cmd_buffer: %s\n", cmd_buffer[buf_tail].job_name);
        // sleep(2);

        // printf("total %d", total);
        /* Move buf_tail forward, this is a circular queue */ 
        if (buf_tail == CMD_BUF_SIZE)
            buf_tail = 0;

        pthread_cond_signal(&cmd_buf_not_full);
        /* Unlok the shared command queue */
        pthread_mutex_unlock(&cmd_queue_lock);
    } /* end for */
}
