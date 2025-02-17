
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

// Function to extract the message before the character '!'
void extractMessage(const char *input, char *output) {
    char delimiter = '!';
    int i = 0, j = 0;

    // Iterate through input string
    while (input[i] != '\0') {
        if (input[i] == delimiter) {
            break; // Stop copying at the delimiter
        }
        output[j++] = input[i];
        i++;
    }

    output[j] = '\0'; // Null-terminate the output string
}


// Function to extract the bot's name after the character '!'
void extractName(const char *input, char *output) {
    char delimiter = '!';
    int i = 0, j = 0;
    int found = 0;

    // Iterate through input string
    while (input[i] != '\0') {
        if (found) {
            // Copy characters after target character
            output[j++] = input[i];
        }
        if (input[i] == delimiter) {
            found = 1; // Start copying from next character
        }
        i++;
    }

    output[j] = '\0'; // Null-terminate the output string
}

// int strlen(const char *s) {
//     int len = 0;
//     while (s[len] != '\0') {
//         len++;
//     }
//     return len;
// }

// strcat function, which appends src to dest
char *strcat(char *dest, const char *src) {
    char *ptr = dest + strlen(dest);
    while (*src) {
        *ptr++ = *src++;
    }
    *ptr = '\0';
    return dest;
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

    //to receive msg from the previous chatbot
    char recvMsg[MAX_MSG_LEN];
    read(fd[myId-1][0], recvMsg, MAX_MSG_LEN);

    char recvdBotName[MAX_MSG_LEN];
    extractName(recvMsg, recvdBotName);

    // Test: Print received message, extracted bot name, my name, and my ID
    printf("Outside loop\n");
    printf("Received message: %s\n", recvMsg);
    printf("Extracted bot name: %s\n", recvdBotName);
    printf("My name: %s\n", myName);
    printf("My ID: %d\n", myId);

    // If the received bot name is not the same as the current bot's name, 
    // pass the message to the next bot
    if (strcmp(recvdBotName, myName) != 0) {
        write(fd[myId][1], recvMsg, MAX_MSG_LEN);
        exit(0);
    }

    //loop
    while(1){
        //to get msg from the previous chatbot (in loop for now)
        

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

                        // Test: Print new bot, received message, extracted bot name, my name, and my ID
                        printf("Inside loop\n");
                        printf("New bot: %s\n", newBot);
                        printf("Received message: %s\n", recvMsg);
                        printf("Extracted bot name: %s\n", recvdBotName);
                        printf("My name: %s\n", myName);
                        printf("My ID: %d\n", myId);

                        // Find the index of the new bot (successor)
                        for (int i = 0; i < botCount; i++) {
                            if (strcmp(newBot, botNames[i]) == 0) {
                                newBotIndex = i;
                                break;
                            }
                        }

                        // If bot exists and is the current bot, continue chat
                        if (strcmp(myName, newBot) == 0) {
                            printf("You are already chatting with %s.\n", botNames[newBotIndex]);
                            newBotIndex = -1; // Reset for next change
                            break;
                        }

                        // If bot exists and is not the current bot, switch
                        if (newBotIndex != -1 && newBotIndex != myId - 1) {
                            //pass the msg to the next bot with a name added
                            newBotIndex = -1; // Reset for next change
                            strcat(msgBuf, "!");
                            strcat(msgBuf, newBot);
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

    // // Testing message and name extraction
    // char testMsg[MAX_MSG_LEN] = "Message/command!Alice";
    // char extractedMsg[MAX_MSG_LEN];
    // char extractedName[MAX_MSG_LEN];
    // extractMessage(testMsg, extractedMsg);
    // extractName(testMsg, extractedName);
    // printf("Extracted message: %s\n", extractedMsg);
    // printf("Extracted name: %s\n", extractedName);

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

    //FIX THIS, THIS IS WHERE THE PROBLEM IS
    // String to hold START!
    char startMsg[MAX_MSG_LEN] = ":START!";
    // Add the first bot name to the string
    strcat(startMsg, argv[1]);

    // Get length of the whole string
    int len = strlen(startMsg);

    // // Test: Print the start message
    // printf("Start message: %s\n", startMsg);

    // // Test: extract command from message
    // char extractedMsg[MAX_MSG_LEN];
    // extractMessage(startMsg, extractedMsg);
    // printf("Extracted message: %s\n", extractedMsg);

    // // Test: extract name from message
    // char extractedName[MAX_MSG_LEN];
    // extractName(startMsg, extractedName);
    // printf("Extracted name: %s\n", extractedName);


    //send START![firstBotName] to the first bot
    write(fd[0][1], startMsg, len);

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
