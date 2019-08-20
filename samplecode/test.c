#include<stdio.h>
#include<pthread.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<time.h>
#include<sys/wait.h>

#define CMD_BUF_SIZE 100           //Job queue size
#define ARR_RATE 1          //ARRIVE RATE IS 1 Job per second

typedef struct 
{
    char name[10];      //job name 
    char elaTime[10];      //burst time
    time_t arrTime;      //arrive time
    time_t staTime;     //start time;
    time_t finTime;      //finish time
    double cpuTime;     //cpu time 
    double watTime;      //waiting time
    int priority;       //pirority

}new_cmd;

typedef struct 
{
    char operation[10];
    char job_name[10];
    char burstTime[10];
    int pri;
    char policy[10];
    int job_num;
    int pri_level;
    int max_cpu;
    int min_cpu;
}command_t;

new_cmd job_queue[CMD_BUF_SIZE];
new_cmd fin_queue[CMD_BUF_SIZE];
int stop=0; //quit signal
int in=0;
int out=0;
int count=0;
int fin_count=0;
int total_job=0;
char sys_policy[10]="fcfs";
pthread_mutex_t queue_mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t not_full_cv=PTHREAD_COND_INITIALIZER;
pthread_cond_t not_empty_cv=PTHREAD_COND_INITIALIZER;

command_t* command_line_parser(char* input)
{
    command_t* command;
    command=(command_t*) malloc(sizeof(command_t));
    memset(command,0,sizeof(command_t));
    sscanf(input,"%s",command->operation);
    if (strcmp(command->operation,"run")==0)
    {  
        sscanf(input,"%s %s %s %d",command->operation,command->job_name,command->burstTime,&command->pri);
        return command;
    }
    else if (strcmp(command->operation,"help")==0)
    {
        return command;
    }
    else if (strcmp(command->operation,"test")==0)
    {
        sscanf(input,"%s %s %s %d %d %d %d",command->operation,command->job_name,command->policy,&command->job_num,&command->pri_level,&command->min_cpu,&command->max_cpu);
        return command;
    }
    else if (strcmp(command->operation,"list")==0)
    {
        return command;
    }
    else if (strcmp(command->operation,"sjf")==0)
    {
        return command;
    }
    else if (strcmp(command->operation,"fcfs")==0)
    {
        return command;
    }
    else if (strcmp(command->operation,"priority")==0)
    {
        return command;
    }
    else if (strcmp(command->operation,"quit")==0)
    {
        return command;
    }
    else
    {
        strcpy(command->operation,"error");
        return command;
    }
}

void fcfs()
{   pthread_mutex_lock(&queue_mutex);
    new_cmd temp;
    int wait_job;
    int i,j;
    if(count>0)
    {
        wait_job=count-1;
    }
    else
    {
        wait_job=0;
    } 
    for(i=out+1;i!=in;i++)
    {
        if(i==CMD_BUF_SIZE)
        {
            i=0;
        }
        for(j=i+1;j!=in;j++)
        {
            if(j==CMD_BUF_SIZE)
            {
                j=0;
            }
            if(job_queue[i].arrTime > job_queue[j].arrTime)  
            {  
                temp = job_queue[i];  
                job_queue[i] = job_queue[j];  
                job_queue[j] = temp;  
            }  
        }
    }
    strcpy(sys_policy,"fcfs");
    pthread_mutex_unlock(&queue_mutex);
    
}
void sjf()
{   
    pthread_mutex_lock(&queue_mutex);
    new_cmd temp;
    int wait_job;
    int i,j;
    if(count>0)
    {
        wait_job=count-1;
    }
    else
    {
        wait_job=0;
    } 
    for(i=out+1;i!=in;i++)
    {
        if(i==CMD_BUF_SIZE)
        {
            i=0;
        }
        for(j=i+1;j!=in;j++)
        {
            if(j==CMD_BUF_SIZE)
            {
                j=0;
            }
            if(atoi(job_queue[i].elaTime)>atoi(job_queue[j].elaTime))  
            {  
                temp = job_queue[i];  
                job_queue[i] = job_queue[j];  
                job_queue[j] = temp;  
            }  
        }
    }
    strcpy(sys_policy,"sjf");
    pthread_mutex_unlock(&queue_mutex);
}
void priority_sort()
{   
    pthread_mutex_lock(&queue_mutex);
    new_cmd temp;
    int wait_job;
    int i,j;
    if(count>0)
    {
        wait_job=count-1;
    }
    else
    {
        wait_job=0;
    } 
    for(i=out+1;i!=in;i++)
    {
        if(i==CMD_BUF_SIZE)
        {
            i=0;
        }
        for(j=i+1;j!=in;j++)
        {
            if(j==CMD_BUF_SIZE)
            {
                j=0;
            }
            if(job_queue[i].priority < job_queue[j].priority)  
            {  
                temp = job_queue[i];  
                job_queue[i] = job_queue[j];  
                job_queue[j] = temp;  
            }  
        }
    }
    strcpy(sys_policy,"priority");
   
    pthread_mutex_unlock(&queue_mutex);
}


void list_queue()
{   
    struct tm *info;
    int i;
    pthread_mutex_lock(&queue_mutex);
    printf("Total number of jobs in the waiting queue:%d\n",count);
    printf("scheduling Policy:%s\n",sys_policy);
    if(count==0)
    {
        printf("Job Queue is currently empty!\n");

    }
    else
    printf("Name\tCPU_Time\tPri\tArr_Time\tStatus\n");
    for(i=out;i!=in;i++)
    {   
        if(i==(CMD_BUF_SIZE))
        {
            i=0;
        }
        printf("%s\t%s\t%10d\t",job_queue[i].name,job_queue[i].elaTime,job_queue[i].priority);
        info=gmtime(&job_queue[i].arrTime);
        
        if(i==out)
        {
            printf("running\n");
        }
        else
        printf("\n");
    }
   
    pthread_mutex_unlock(&queue_mutex);
}

void help()
{
    printf("run<job><time> <pri>:submit a job named <job>, execution time is <time>, priority is <pri>.\nlist:display the job status.\nfcfs: change the scheduling policy to FCFS.\nsjf: change the scheduling policy to SJF.\npriority: change the scheduling policy to priority.\ntest <benchmark><policy> <num_of_jobs><priority_levels> <min_CPU_time><max_CPU_time>\nquit: exit AUbatch\n");
}



void *scheduler(void *argv)
{   
    double EstTime=0;
    int i,j;
    time_t now;
    command_t *new_command=(command_t*)argv;
    if(strcmp(new_command->operation,"run")==0)
    {  
        pthread_mutex_lock(&queue_mutex);
        while(count==CMD_BUF_SIZE)
        {
            printf("The Job Queue is full, waiting for dispatch\n");
            pthread_cond_wait(&not_full_cv,&queue_mutex);
        }
        if(count==0)
        {
            pthread_cond_signal(&not_empty_cv);
        }
        count++;
        strcpy(job_queue[in].name,new_command->job_name);
        strcpy(job_queue[in].elaTime,new_command->burstTime);
        job_queue[in].priority=new_command->pri;
        job_queue[in].arrTime=time(NULL);
        printf("Job <%s> was submitted.\n ",job_queue[in].name);
        printf("Total job in the queue:%d.\n",count);  
        for(i=out;i!=in;i++)
        { 
            if(i==CMD_BUF_SIZE)
             {
                i=0;
             }
            EstTime+=atoi(job_queue[i].elaTime);
        }
        in++;
        if(in==CMD_BUF_SIZE)
        {
            in=0;
        }
        pthread_mutex_unlock(&queue_mutex);
        if(strcmp(sys_policy,"sjf")==0)
        {
            sjf();
        }
        else if(strcmp(sys_policy,"priority")==0)
        {
            priority_sort();
        }
        else if(strcmp(sys_policy,"fcfs")==0)
        {
            fcfs();
        }
        printf("Expected waiting time:%.0f\n",EstTime);
    }
    else if(strcmp(new_command->operation,"sjf")==0)
        {
            sjf();
            printf("Scheduling policy is switch to sjf\n");
        }
    else if(strcmp(new_command->operation,"fcfs")==0)
        {
            fcfs();
            printf("Scheduling policy is switch to fcfs\n");
        }
    else if(strcmp(new_command->operation,"priority")==0)
        {
            priority_sort();
            printf("Scheduling policy is switch to priority.\n");
        }
    else if(strcmp(new_command->operation,"test")==0)
        {
            printf("auto test module\n");
            pthread_mutex_lock(&queue_mutex);
            fin_count=0;
            total_job=new_command->job_num;
            pthread_mutex_unlock(&queue_mutex);
            
            printf("Test Information:\nThe benchmark name is: %s\nPolicy is:%s\nNumber of job: %d\n priority levels are %d\nMIN_CPU time: %d\nMAX_CPU time: %d\n",new_command->job_name,new_command->policy,new_command->job_num,new_command->pri_level,new_command->min_cpu,new_command->max_cpu);
            printf("default arrive rate is:%dNo/sec\n",ARR_RATE);
            srand(time(NULL));
            for(j=0;j<new_command->job_num;j++)
            {   
                pthread_mutex_unlock(&queue_mutex);
                strcpy(sys_policy,new_command->policy);
                while(count==CMD_BUF_SIZE)
                {
                    pthread_cond_wait(&not_full_cv,&queue_mutex);
                }
                if(count==0)
                {
                    pthread_cond_signal(&not_empty_cv);
                }
                count++;
                strcpy(job_queue[in].name,new_command->job_name);
                sprintf(job_queue[in].elaTime,"%d",((rand())%(new_command->max_cpu-new_command->min_cpu)+(new_command->min_cpu)));
                job_queue[in].priority=rand()%new_command->pri_level;
                job_queue[in].arrTime=time(NULL);
                in++;
                if(in==CMD_BUF_SIZE)
                {
                    in=0;
                }
                pthread_mutex_unlock(&queue_mutex);
                if(strcmp(sys_policy,"sjf")==0)
                {
                    sjf();
                }
                else if(strcmp(sys_policy,"priority")==0)
                {
                    priority_sort();
                }
                else if(strcmp(sys_policy,"fcfs")==0)
                {
                    fcfs();
                } 
                pthread_mutex_unlock(&queue_mutex);
                sleep(ARR_RATE);
        
            }
        printf("Finished scheduling all %d test job, Please wait for result.\n",new_command->job_num);
        }
    pthread_exit(NULL);
}
void *dispatcher(void *argv)
{ 
    pid_t pid;
    pid_t temp;
    int i;
    char *exe[3];
    
    while(stop==0)
    {
        pthread_mutex_lock(&queue_mutex);
        while(count==0)
        {
            printf("The Job Queue is currently empty, waiting for new job\n");
            pthread_cond_wait(&not_empty_cv,&queue_mutex);
           
        }
        if(stop==1)
        {
            pthread_mutex_unlock(&queue_mutex);
            break;
        }
        exe[0]=job_queue[out].name;
        exe[1]=job_queue[out].elaTime;    
        exe[2]=NULL;
        job_queue[out].staTime=time(NULL);
        pthread_mutex_unlock(&queue_mutex);
        switch((pid=fork()))
        {
            case -1:
            perror("fork");
            break;
            case 0:     //child process replace
            execv(exe[0],exe);
            puts("Error during execv()");
            exit(-1);
            break;
            default:  //parent process print 
            break;
        }
        wait(NULL);
        pthread_mutex_lock(&queue_mutex);
        job_queue[out].finTime=time(NULL);
        job_queue[out].cpuTime=difftime(job_queue[out].finTime,job_queue[out].staTime);
        job_queue[out].watTime=difftime(job_queue[out].staTime,job_queue[out].arrTime);
        if(count==CMD_BUF_SIZE)
        {
            pthread_cond_signal(&not_full_cv);
        }
        count--;
        memcpy(fin_queue+fin_count,job_queue+out,sizeof(new_cmd)); //move job to finished queue
        fin_count++;
        out++;
        if(out==CMD_BUF_SIZE)
        {
            out=0;
        }
        if(fin_count==total_job)
        {   
            float avg_turnarond=0;
            float avg_cpu=0;
            float avg_wait=0;
            float throughput=0;
            for(i=0;i<total_job;i++)
            {
                avg_cpu+=(fin_queue[i].cpuTime/total_job);
                avg_wait+=(fin_queue[i].watTime/total_job);
            }
            avg_turnarond=avg_cpu+avg_wait;
            printf("Total number of job finished:%d\n",total_job);
            printf("Average Turn around Time:%3.3f\n",avg_turnarond);
            printf("Average CPU Time:%3.3f\n",avg_cpu);
            printf("Average waiting Time:%3.3f\n",avg_wait);
            printf("Throughput:%3.3f\n",1/avg_turnarond);

        }
        pthread_mutex_unlock(&queue_mutex);
    }
    pthread_exit(NULL);
}


int main()
{
    int i;
    char c;
    pthread_t tid[2];
    command_t *input_command=NULL;
    char input_buff[100];
    pthread_create(&tid[1],NULL,dispatcher,NULL);
    printf("Type help to find more about AUbatch commands.\n");
    while(1)
    {   
        printf(">");
        scanf("%[^\n]",input_buff);
        c=getchar();
        input_command=command_line_parser(input_buff);
        if(strcmp(input_command->operation,"run")==0)
        {
             pthread_create(&tid[0],NULL,scheduler,(void* )input_command);
             pthread_join(tid[0],NULL); 
        }
        if(strcmp(input_command->operation,"list")==0)
        {
             list_queue();
        }
        if(strcmp(input_command->operation,"help")==0)
        {
             help();
        }
       if(((strcmp(input_command->operation,"sjf"))&&(strcmp(input_command->operation,"fcfs"))&&(strcmp(input_command->operation,"priority")))==0)
        {    
            pthread_create(&tid[0],NULL,scheduler,(void* )input_command);
            pthread_join(tid[0],NULL); 
        } 
        if(strcmp(input_command->operation,"test")==0)
        {
            pthread_create(&tid[0],NULL,scheduler,(void* )input_command);
            pthread_join(tid[0],NULL); 
        }
        if(strcmp(input_command->operation,"error")==0)
        {
            printf("Unknown command! Type <help> to get more information\n");
        }
        if(strcmp(input_command->operation,"quit")==0)
        {   
            
            printf("System is closing\n");
            stop=1;
            float avg_turnarond=0;
            float avg_cpu=0;
            float avg_wait=0;
            float throughput=0;
          
            for(i=0;i<fin_count;i++)
            {
                avg_cpu+=(fin_queue[i].cpuTime/fin_count);
                avg_wait+=(fin_queue[i].watTime/fin_count);
            }
            avg_turnarond=avg_cpu+avg_wait;
            printf("Total number of job finished:%d\n",fin_count);
            printf("Average Turn around Time:%3.3f\n",avg_turnarond);
            printf("Average CPU Time:%3.3f\n",avg_cpu);
            printf("Average waiting Time:%3.3f\n",avg_wait);
            printf("Throughput:%3.3f\n",1/avg_turnarond);
            pthread_cond_signal(&not_empty_cv);

            break;

        }
    }
    return 0;
}