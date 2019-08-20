#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
    double exetime;
    if(argc=!2)
    {
        perror("JobBenchmark error:");

    }
    else
    {
        exetime=atoi(argv[1]);
        printf("exetime %d",exetime);   
        sleep(exetime);
        
    }
 return 0; 
}