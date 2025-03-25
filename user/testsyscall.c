#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
panic(char *s)
{
  fprintf(2, "%s\n", s);
  exit(1);
}

int
fork1(void)
{
  int pid;
  pid = fork();
  if(pid == -1)
    panic("fork");
  return pid;
}


int
main(int argc, char* argv[])
{

	//set a pipe to make processes output mutually exclusive 
	int fd[2];
	if (pipe(fd) == -1) panic("pipe");
	char msg[128]="Hi";

	//set up parent process and start cfs
	nice(3);
	startcfs(100,20,2);
	printf("\n[START] Process (pid:%d) has started cfs!\n\n", getpid());

	//create 10 child processes: first 5 have lower priority than the last 5
        int ret=0;
        for(int i=0; i<10; i++){
                ret=fork1();
                if(ret==0){
                        if(i<5) nice(10);
			else nice(5);
                        break;
                }
        }
	int mypid = getpid();
        printf("[PRIORITY] process (pid=%d): has nice = %d\n", mypid,nice(-30));


	//do intensive computation
        int t=0;
        while(t++<2){
                double x=987654321.9;
                for(int i=0; i<100000000; i++){
                        x /= 12345.6789;
                }
        }


	//when parent is done (which should be earlier than children due to its high priority), it stop CFS and then every one summarize their actual and virtual runtime during CFS
	if(ret>0){
		stopcfs();
		printf("\n[STOP] Process (pid=%d): has stopped cfs\n\n", mypid);
		write(fd[1],msg,128);
		for(int i=0; i<10; i++) wait(0);
	}


	//every one summarizies its actual and virtual runtime during CFS; note: pipe is used for mutual exclusion
	read(fd[0],msg,128);
	int runtime[2];
        if(getruntime(&runtime[0], &runtime[1])==0)
            printf("[SUMMARY] process (pid=%d): finishes comutation. During CFS: actual runtime = %d; virtual runtime = %d\n", mypid, runtime[0], runtime[1]);
        else
            printf("\n [ERROR] Process (pid=%d): something wrong with getruntime!\n\n", mypid);
	write(fd[1],msg,128);

        return 0;
}


