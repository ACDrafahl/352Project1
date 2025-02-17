
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define MAX_MSG_LEN 512
#define MAX_NUM_CHATBOT 5

int fd[MAX_NUM_CHATBOT+1][2]; // Pipes for chatbots
int botCount = 0; // Number of chatbots
char *botNames[MAX_NUM_CHATBOT+1]; // Store bot names
int currentbot = 0; // Current bot index

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

// Function to extract substring after a specific character
void extractSubstringAfterChar(const char *input, char target, char *output) {
    int i = 0, j = 0;
    int found = 0;

    // Iterate through input string
    while (input[i] != '\0') {
        if (found) {
            // Copy characters after target character
            output[j++] = input[i];
        }
        if (input[i] == target) {
            found = 1; // Start copying from next character
        }
        i++;
    }

    output[j] = '\0'; // Null-terminate the output string
}

/* NOTE: the reworking of the chatbot function isn't complete, but it was
meant to be a simple way to swith between bots without the need to fully strip 
away the ring structure. The idea was to have the bot name in the message (hence the
extractSubstringAfterChar function) and then check if the bot name matches the current bot's
name. That would allow you to use the change command and then request a specific bot, 
which would go through the existing pipes in the ring until it found the correct bot. I 
had to switch to this solution last minute, since the other approach I had tried eventually
became too complicated to finish in time. As such, this version is incomplete, but I hope you can 
see what I was going for. 
*/

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
        //to get msg from the previous chatbot (in loop for now)
        char recvMsg[MAX_MSG_LEN];
        read(fd[myId-1][0], recvMsg, MAX_MSG_LEN);

        // // if the name after the character [ is not the same as the bot name, go to the next bot
        // char botName[MAX_MSG_LEN];
        // extractSubstringAfterChar(recvMsg, '[', botName);
        // if (strcmp(botName, myName) != 0) {
        //     write(fd[myId][1], recvMsg, MAX_MSG_LEN);
        //     continue;
        // }

        // Indicates the index of the new bot
        int newBotIndex = -1;

        if(strcmp(recvMsg,":EXIT")!=0 && strcmp(recvMsg,":exit")!=0){//if the received msg is not EXIT/exit: continue chatting 
            
            printf("Hello, this is chatbot %s. Please type:\n", myName);

            while (1) { // keep chatting until user enters :CHANGE/:change or :EXIT/:exit
                //get a string from std input and save it to msgBuf 
                char msgBuf[MAX_MSG_LEN];
                gets1(msgBuf);
                
                printf("I heard you said: %s\n", msgBuf);

                // If user inputs CHANGE/change: validate bot and switch
                if(strcmp(msgBuf,":CHANGE") == 0 || strcmp(msgBuf,":change") == 0) {
                    while (newBotIndex == -1) {
                        // Prompt user for new bot name
                        printf("Which bot would you like to chat with? ");
                        char newBot[MAX_MSG_LEN];
                        gets1(newBot);

                        // Find the index of the new bot (successor)
                        for (int i = 0; i < botCount; i++) {
                            if (strcmp(newBot, botNames[i]) == 0) {
                                newBotIndex = i;
                                break;
                            }
                        }

                        // If bot exists and is the current bot, continue chat
                        if (newBotIndex == myId - 1) {
                            printf("You are already chatting with %s.\n", botNames[newBotIndex]);
                            newBotIndex = -1; // Reset for next change
                            break;
                        }

                        if (newBotIndex != -1 && newBotIndex != myId - 1) {
                            //pass the msg to the next bot with a name added
                            newBotIndex = -1; // Reset for next change
                            write(fd[myId][1], msgBuf, MAX_MSG_LEN); 
                            break;
                        }

                        // If bot doesn't exist, prompt again
                        printf("Bot %s does not exist. Please try again.\n", newBot);
                    }
                } else if(strcmp(msgBuf,":EXIT")==0 || strcmp(msgBuf,":exit")==0) {
                    //if user inputs EXIT/exit: exit myself
                    //pass the msg to the next bot
                    write(fd[myId][1], msgBuf, MAX_MSG_LEN); 
                    exit(0);
                } else {
                    printf("Please continue typing:\n");
                }
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
    if(argc < 3 || argc > MAX_NUM_CHATBOT + 1) {
        printf("Usage: %s <list of names for up to %d chatbots>\n", argv[0], MAX_NUM_CHATBOT);
        exit(1);
    }

    // Bot count
    botCount = argc - 1;

    pipe1(fd[0]); //create the first pipe #0
    // Create pipes and store names
    for(int i = 1; i < argc; i++) {
        pipe1(fd[i]); //create one new pipe for each chatbot
        botNames[i - 1] = argv[i]; // Store bot name
        //to create child proc #i (emulating chatbot #i)
    }

    // Create child processes
    for(int i = 1; i < argc; i++) {
        if(fork1() == 0) {
            chatbot(i, argv[i]);
        }
    }

    //close the fds not used any longer
    close(fd[0][0]); 
    close(fd[argc - 1][1]);
    for(int i = 1; i < argc - 1; i++) {
        close(fd[i][0]);
        close(fd[i][1]);
    }

    //send the START msg to the first chatbot
    write(fd[0][1], ":START", 6);

    //loop: when receive a token from predecessor, pass it to successor
    while(1) {
        char recvMsg[MAX_MSG_LEN];
        read(fd[argc-1][0], recvMsg, MAX_MSG_LEN); 
        write(fd[0][1], recvMsg, MAX_MSG_LEN);
	    if(strcmp(recvMsg,":EXIT")==0||strcmp(recvMsg,":exit")==0) break; //break from the loop if the msg is EXIT
    }

    //exit after all children exit
    for(int i=1; i<=argc; i++) {
        wait(0);
    }
    printf("Now the chatroom closes. Bye bye!\n");
    exit(0);

}
