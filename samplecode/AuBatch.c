/*
 * COMP7500/7506
 * Project 3: commandline_parser.c
 *
 *
 * Compilation Instruction:
 * gcc commandline_parser.c -o commandline_parser
 * ./commandline_parser
 *
 */

#include <sys/types.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <time.h>

typedef unsigned int u_int;

/* Error Code */
#define EINVAL       1
#define E2BIG        2

#define CMD_BUF_SIZE 10   /* The size of the command queueu */
#define NUM_OF_CMD   5    /* The number of submitted jobs   */
#define MAX_CMD_LEN  512  /* The longest commandline length */

#define MAXMENUARGS  4 
#define MAXCMDLINE   64 

void menu_execute(char *line, int isargs); 
int cmd_run(int nargs, char **args);
int cmd_list(int nargs, char **args);
int cmd_fcfs(int nargs, char **args);
int cmd_sjf(int nargs, char **args);
int cmd_pri(int nargs, char **args);
int cmd_quit(int nargs, char **args); 
void showmenu(const char *name, const char *x[]);
int cmd_helpmenu(int n, char **a);
int cmd_dispatch(char *cmd);
void *commandline(void *ptr);  /* To simulate job submissions and scheduling */
void *executor(void *ptr);     /* To simulate job execution */
void rearrange(void);


pthread_mutex_t cmd_queue_lock;  /* Lock for critical sections */
pthread_cond_t cmd_buf_not_full; /* Condition variable for buf_not_full */
pthread_cond_t cmd_buf_not_empty; /* Condition variable for buf_not_empty */

/* Global shared variables */
u_int buf_head = 0;
u_int new_job = 0;
u_int buf_tail;
u_int count;
u_int sign = 0;
u_int policy = 0; //0: FCFS; 1: SJF; 2: priority
struct{
    u_int hour, min, sec, exe_time, priority;
    char name[32];
}cmd_array[CMD_BUF_SIZE], cmd_swap;

char *cmd_buffer[CMD_BUF_SIZE];

/*
 * The run command - submit a job.
 */
int cmd_run(int nargs, char **args) {
	if (nargs != 4) {
		printf("Usage: run <job> <time> <priority>\n");
		return EINVAL;
	}

    time_t timep;
    int sum_time = 0;
    struct tm *p;
    time(&timep);
    p = gmtime(&timep);

    cmd_array[count].sec  = p->tm_sec;
    cmd_array[count].min  = p->tm_min;
    cmd_array[count].hour = p->tm_hour;
    
    strcpy(cmd_array[count].name, args[1]);
    cmd_array[count].exe_time = atoi(args[2]);
    cmd_array[count].priority = atoi(args[3]);

    //count ++;
    
    printf("Job %s was submitted.\n", args[1]);
    printf("Total number of jobs in the queue: %d\n", count - buf_head);
    
    if(count >= 1)
        for(int i = buf_head + 1; i <= count; i ++)
            sum_time += cmd_array[i].exe_time;
    
    printf("Expected waiting time: %d seconds\n", sum_time);
    printf("Scheduling Policy: ");
    switch(policy)
    {
        case 0: printf("FCFS.\n");break;
        case 1: printf("SJF.\n");break;
        case 2: printf("Priority.\n");break;
    }
    new_job = 1;
    /* Use execv to run the submitted job in AUbatch */
    //printf("use execv to run the job in AUbatch.\n");
    return 0; /* if succeed */
}

/*
 * The list command - submit a job.
 */
int cmd_list(int nargs, char **args) {
    printf("Total number of jobs in the queue: %d\n", count - buf_head);
    printf("Scheduling Policy: ");
    switch(policy)
    {
        case 0: printf("FCFS.\n");break;
        case 1: printf("SJF.\n");break;
        case 2: printf("Priority.\n");break;
    }
    
    printf("Name    CPU_Time   Pri    Arrival_time  Progress\n");
    for(int i = buf_head; i < count; i ++)
    {
        printf("%s         %d       %d       %d:%d:%d", cmd_array[i].name, cmd_array[i].exe_time,
            cmd_array[i].priority, cmd_array[i].hour, cmd_array[i].min, cmd_array[i].sec);
        if(i == buf_head)
            printf("     run");
        
        printf("\n");
    }
}
/*
 * The fcfs command - submit a job.
 */
int cmd_fcfs(int nargs, char **args) {
    policy = 0;
    rearrange();
}

/*
 * The sjf command - submit a job.
 */
int cmd_sjf(int nargs, char **args) {
    policy = 1;
    rearrange();
}

/*
 * The pri command - submit a job.
 */
int cmd_pri(int nargs, char **args) {
    policy = 2;
    rearrange();
}

/*
 * The quit command.
 */
int cmd_quit(int nargs, char **args) {
	printf("Please display performance information before exiting AUbatch!\n");
        exit(0);
}

/*
 * Display menu information
 */
void showmenu(const char *name, const char *x[])
{
	int ct, half, i;

	printf("\n");
	printf("%s\n", name);
	/*
	for (i=ct=0; x[i]; i++) {
		ct++;
	}
	half = (ct+1)/2;*/

	for (i = 0; i < strlen(x) ; i ++) {
		printf("%s\n", x[i]);
	}
    
    printf("\n");
}

static const char *helpmenu[] = {
    "run <job> <time> <priority>: submit a job named <job>, execution time is <time>, priority is <pri>",
    "list: display the job status.",
    "fcfs: change the scheduling policy to FCFS.",
    "sjf: change the scheduling policy to SJF.",
    "priority: change the scheduling policy to priority.",
    "quit: Exit AUbatch",
        /* Please add more menu options below */
	NULL
};

int cmd_helpmenu(int n, char **a)
{
	(void)n;
	(void)a;

	showmenu("AUbatch help menu", helpmenu);
	return 0;
}

/*
 *  Command table.
 */
static struct {
	const char *name;
	int (*func)(int nargs, char **args);
} cmdtable[] = {
	/* commands: single command must end with \n */
	{ "help\n",	    cmd_helpmenu },
	{ "run",	    cmd_run },
    { "list\n",     cmd_list },
    { "fcfs\n",     cmd_fcfs },
    { "sjf\n",      cmd_sjf },
    { "priority\n", cmd_pri },
	{ "quit\n",	    cmd_quit },
        /* Please add more operations below. */
        {NULL, NULL}
};

/*
 * Process a single command.
 */
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

	printf("%s: Command not found\n", args[0]);
	return EINVAL;
}

/*
 *Re-arrange the job sequence
 */
void rearrange(void)
{
    int i, j;
    char *swap1, *swap2;
    //printf("%d, %d\n", buf_head, count);
    if(count - buf_head <= 2)
        return ;
    
    //printf("%d\n", policy);
    switch(policy)
    {
        case 0: //FCFS
            for(i = buf_head + 1; i < count - 1; i ++)
                for(j = buf_head + 1; j < count - 1; j ++)
                    if((cmd_array[j].hour > cmd_array[j + 1].hour) || ((cmd_array[j].hour == cmd_array[j + 1].hour) && (cmd_array[j].min > cmd_array[j + 1].min)) ||
                       ((cmd_array[j].hour == cmd_array[j + 1].hour) && (cmd_array[j].min == cmd_array[j + 1].min) && (cmd_array[j].sec > cmd_array[j + 1].sec)))
                    {
                        cmd_swap = cmd_array[j];
                        cmd_array[j] = cmd_array[j + 1];
                        cmd_array[j + 1] = cmd_swap;
                        
                        swap1 = malloc(MAX_CMD_LEN*sizeof(char));
                        swap2 = malloc(MAX_CMD_LEN*sizeof(char));
                        
                        strcpy(swap1, cmd_buffer[j + 1]);
                        strcpy(swap2, cmd_buffer[j]);
                        free(cmd_buffer[j]);
                        free(cmd_buffer[j + 1]);
                        cmd_buffer[j] = swap1;
                        cmd_buffer[j + 1] = swap2;
                    }
            break;
        case 1: //SJF
            for(i = buf_head + 1; i < count - 1; i ++)
                for(j = buf_head + 1; j < count - 1; j ++)
                {
                    if(cmd_array[j].exe_time > cmd_array[j + 1].exe_time)
                    {
                        cmd_swap = cmd_array[j];
                        cmd_array[j] = cmd_array[j + 1];
                        cmd_array[j + 1] = cmd_swap;
                        
                        swap1 = malloc(MAX_CMD_LEN*sizeof(char));
                        swap2 = malloc(MAX_CMD_LEN*sizeof(char));
                        
                        strcpy(swap1, cmd_buffer[j + 1]);
                        strcpy(swap2, cmd_buffer[j]);
                        free(cmd_buffer[j]);
                        free(cmd_buffer[j + 1]);
                        cmd_buffer[j] = swap1;
                        cmd_buffer[j + 1] = swap2;

                    }
                }
            break;
        case 2: //priority
            for(i = buf_head + 1; i < count - 1; i ++)
                for(j = buf_head + 1; j < count - 1; j ++)
                    if(cmd_array[j].priority > cmd_array[j + 1].priority)
                    {
                        cmd_swap = cmd_array[j];
                        cmd_array[j] = cmd_array[j + 1];
                        cmd_array[j + 1] = cmd_swap;
                        
                        swap1 = malloc(MAX_CMD_LEN*sizeof(char));
                        swap2 = malloc(MAX_CMD_LEN*sizeof(char));
                        
                        strcpy(swap1, cmd_buffer[j + 1]);
                        strcpy(swap2, cmd_buffer[j]);
                        free(cmd_buffer[j]);
                        free(cmd_buffer[j + 1]);
                        cmd_buffer[j] = swap1;
                        cmd_buffer[j + 1] = swap2;
                    }
            break;
    }
}


/*
 * This function simulates a terminal where users may
 * submit jobs into a batch processing queue.
 * Note: The input parameter (i.e., *ptr) is optional.
 * If you intend to create a thread from a function
 * with input parameters, please follow this example.
 */
void *commandline(void *ptr) {
    char *message;
    char *temp_cmd, *temp_num;
    u_int i, j;
    char num_str[8];
    char temp_cmd_new[512];
    
    size_t command_size;
    
    char *buffer;
    size_t bufsize = 64;
    
    buffer = (char*) malloc(bufsize * sizeof(char));
    if (buffer == NULL) {
        perror("Unable to malloc buffer");
        exit(1);
    }
    
    /*
    while(1)
    {
        printf("Welcome to Jueting's batch job scheduler Version 1.0\nType 'help' to find more about AUbatch commands.\n>");
        getline(&buffer, &bufsize, stdin);
        cmd_dispatch(buffer);
        
    }
     */
    while(1)
    {
        printf("Welcome to Jueting's batch job scheduler Version 1.0\nType 'help' to find more about AUbatch commands.\n>");
        getline(&buffer, &bufsize, stdin);
        new_job = 0;
        cmd_dispatch(buffer);
    
        if(new_job == 1)// && sign == 0)
        {
            while(count == CMD_BUF_SIZE)
            {
                pthread_mutex_lock(&cmd_queue_lock);
                pthread_cond_wait(&cmd_buf_not_full, &cmd_queue_lock);
                pthread_mutex_unlock(&cmd_queue_lock);
            }
            sign = 1;
            new_job = 0;
            rearrange();
    
            memset(temp_cmd_new, 0, sizeof(temp_cmd_new));
            temp_cmd_new[0] = '.';
            temp_cmd_new[1] = '/';
            temp_cmd_new[2] = 'p';
            temp_cmd_new[3] = 'r';
            temp_cmd_new[4] = 'o';
            temp_cmd_new[5] = 'c';
            temp_cmd_new[6] = 'e';
            temp_cmd_new[7] = 's';
            temp_cmd_new[8] = 's';
            temp_cmd_new[9] = ' ';
            
            temp_num = malloc(MAX_CMD_LEN*sizeof(char));
            temp_cmd = malloc(MAX_CMD_LEN*sizeof(char));
            sprintf(temp_num, "%d", cmd_array[count].priority);
            for(i = 0; i < strlen(temp_num); i ++)
                temp_cmd_new[10 + i] = temp_num[i];
            temp_cmd_new[10 + i] = ' ';
            sprintf(temp_num, "%d", cmd_array[count].exe_time);
            for(j = 0; j < strlen(temp_num); j ++)
                temp_cmd_new[11 + i + j] = temp_num[j];
            temp_cmd_new[11 + i + j] = '\n';
            free(temp_num);
            strcpy(temp_cmd, temp_cmd_new);
            //printf("In commandline: %s\n", temp_cmd);
            pthread_mutex_lock(&cmd_queue_lock);
            cmd_buffer[count]= temp_cmd;
            count ++;
            //printf("commander : %s,%d\n",cmd_buffer[count - 1], count - 1);
            pthread_cond_signal(&cmd_buf_not_empty);
            pthread_mutex_unlock(&cmd_queue_lock);
            sleep(2);
            
        }
    }
}

/*
 * This function simulates a server running jobs in a batch mode.
 * Note: The input parameter (i.e., *ptr) is optional.
 * If you intend to create a thread from a function
 * with input parameters, please follow this example.
 */
void *executor(void *ptr) {
    char *message;
    u_int i, j, sum_space, index;
    char exe_time[512];
    
    
   while(1)
   {
       if(count == 0)
       {
           pthread_mutex_lock(&cmd_queue_lock);
           pthread_cond_wait(&cmd_buf_not_empty, &cmd_queue_lock);
           pthread_mutex_unlock(&cmd_queue_lock);
       }
       else
       {
            if(buf_head == count)
                continue;
            memset(exe_time, 0, sizeof(exe_time));
            sum_space = 0;
            index = 0;
            pthread_mutex_lock(&cmd_queue_lock);
            //printf("executer: %s, %d,%d\n", cmd_buffer[buf_head], buf_head, count);
            for(i = 0; i < strlen(cmd_buffer[buf_head]); i ++)
            {
                if(sum_space == 2)
                    exe_time[index ++] = cmd_buffer[buf_head][i];
                
                if(cmd_buffer[buf_head][i] == ' ' || cmd_buffer[buf_head][i] == '\n')
                    sum_space ++;
            }
           //printf("%s", cmd_buffer[buf_head]);
           system(cmd_buffer[buf_head]);
           free(cmd_buffer[buf_head]);
        
           pthread_cond_signal(&cmd_buf_not_full);
           /* Unlok the shared command queue */
           pthread_mutex_unlock(&cmd_queue_lock);
           sleep(atoi(exe_time));
           buf_head ++;
       }
   }

}

/*
 * Command line main loop.
 */
int main()
{
    pthread_t command_thread, executor_thread; /* Two concurrent threads */
    char *message1 = "Command Thread";
    char *message2 = "Executor Thread";
    int  iret1, iret2;
    
    /*
	char *buffer;
    size_t bufsize = 64;*/
    
    /* Initilize count, two buffer pionters */
    count = 0;
    buf_head = 0;
    buf_tail = 0;
    
    /*
    buffer = (char*) malloc(bufsize * sizeof(char));
    if (buffer == NULL) {
    perror("Unable to malloc buffer");
    exit(1);
	}*/
    
    /* Create two independent threads:command and executors */
    iret1 = pthread_create(&command_thread, NULL, commandline, (void*) message1);
    iret2 = pthread_create(&executor_thread, NULL, executor, (void*) message2);
    
    /* Initialize the lock the two condition variables */
    pthread_mutex_init(&cmd_queue_lock, NULL);
    pthread_cond_init(&cmd_buf_not_full, NULL);
    pthread_cond_init(&cmd_buf_not_empty, NULL);
    
    /* Wait till threads are complete before main continues. Unless we  */
    /* wait we run the risk of executing an exit which will terminate   */
    /* the process and all threads before the threads have completed.   */
    pthread_join(command_thread, NULL);
    pthread_join(executor_thread, NULL);
    
    /*
    while (1) {
		//printf("> [? for menu]: ");
        printf("Welcome to Jueting's batch job scheduler Version 1.0\nType 'help' to find more about AUbatch commands.\n>");
		getline(&buffer, &bufsize, stdin);
		cmd_dispatch(buffer);
	}
     */
    return 0;
}
