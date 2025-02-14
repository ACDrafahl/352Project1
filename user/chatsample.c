
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MAX_MSG_LEN 512
#define MAX_NUM_CHATBOT 10

int fd[MAX_NUM_CHATBOT+1][2];


//handle exception
void
panic(char *s)
{
  fprintf(2, "%s\n", s);
  exit(1);
}


//create a new process
int
fork1(void)
{
  int pid;
  pid = fork();
  if(pid == -1)
    panic("fork");
  return pid;
}

//create a pipe
void
pipe1(int fd[2])
{
 int rc = pipe(fd);
 if(rc<0){
   panic("Fail to create a pipe.");
 }
}

//get a string from std input and save it to msgBuf
void
gets1(char msgBuf[MAX_MSG_LEN]){
    gets(msgBuf,MAX_MSG_LEN);
	int len = strlen(msgBuf);
	msgBuf[len-1]='\0';
}


//script for chatbot (child process)
void
chatbot(int myId, char *myName)
{
    //close un-used pipe descriptors
    for(int i=0; i<myId-1; i++){
        close(fd[i][0]);
        close(fd[i][1]);
    }
    close(fd[myId-1][1]);
    close(fd[myId][0]);


    //loop
    while(1){

        //to get msg from the previous chatbot
        char recvMsg[MAX_MSG_LEN];
        read(fd[myId-1][0], recvMsg, MAX_MSG_LEN);

	    if(strcmp(recvMsg,":EXIT")!=0 && strcmp(recvMsg,":exit")!=0){//if the received msg is not EXIT/exit: continue chatting 
            
	        printf("Hello, this is chatbot %s. Please type:\n", myName);
            
	        //get a string from std input and save it to msgBuf 
	        char msgBuf[MAX_MSG_LEN];
            gets1(msgBuf);
            
            printf("I heard you said: %s\n", msgBuf);
            
            //pass the msg to the next one on the ring
	        write(fd[myId][1], msgBuf, MAX_MSG_LEN); 

            //if user inputs EXIT/exit: exit myself
            if(strcmp(msgBuf,"EXIT")==0||strcmp(msgBuf,"exit")==0){
                exit(0);
            } 

        }else{//if receives EXIT/exit: pass the msg down and exit myself
            write(fd[myId][1], recvMsg, MAX_MSG_LEN);
            exit(0);    
        }
            
    }

}



//script for parent process
int
main(int argc, char *argv[])
{
    if(argc<3||argc>MAX_NUM_CHATBOT+1){
        printf("Usage: %s <list of names for up to %d chatbots>\n", argv[0], MAX_NUM_CHATBOT);
        exit(1);
    }

    pipe1(fd[0]); //create the first pipe #0
    for(int i=1; i<argc; i++){
        pipe1(fd[i]); //create one new pipe for each chatbot
        //to create child proc #i (emulating chatbot #i)
        if(fork1()==0){
            chatbot(i,argv[i]);
        }    
    }

    //close the fds not used any longer
    close(fd[0][0]); 
    close(fd[argc-1][1]);
    for(int i=1; i<argc-1; i++){
        close(fd[i][0]);
        close(fd[i][1]);
    }

    //send the START msg to the first chatbot
    write(fd[0][1], ":START", 6);

    //loop: when receive a token from predecessor, pass it to successor
    while(1){
        char recvMsg[MAX_MSG_LEN];
        read(fd[argc-1][0], recvMsg, MAX_MSG_LEN); 
        write(fd[0][1], recvMsg, MAX_MSG_LEN);
	    if(strcmp(recvMsg,":EXIT")==0||strcmp(recvMsg,":exit")==0) break; //break from the loop if the msg is EXIT
    }

    //exit after all children exit
    for(int i=1; i<=argc; i++) wait(0);
    printf("Now the chatroom closes. Bye bye!\n");
    exit(0);

}


