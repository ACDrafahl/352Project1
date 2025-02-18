
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

// strcat function, which appends src to dest
char *strcat(char *dest, const char *src) {
    char *ptr = dest + strlen(dest);
    while (*src) {
        *ptr++ = *src++;
    }
    *ptr = '\0';
    return dest;
}

void chatbot(int myId, char *myName) {
    // Close unused pipe descriptors
    for (int i = 0; i < myId - 1; i++) {
        close(fd[i][0]);
        close(fd[i][1]);
    }
    close(fd[myId - 1][1]);
    close(fd[myId][0]);

    while (1) {
        // Receive message from the previous chatbot
        char recvMsg[MAX_MSG_LEN];
        read(fd[myId - 1][0], recvMsg, MAX_MSG_LEN);

        // Extract the bot name from the received message
        char recvdBotName[MAX_MSG_LEN];
        extractName(recvMsg, recvdBotName);

        // If the received bot name is not the same as the current bot's name, pass the message to the next bot
        if (strcmp(recvdBotName, myName) != 0) {
            write(fd[myId][1], recvMsg, MAX_MSG_LEN);
            exit(0);
        }

        // If the message is for the current bot, handle user input
        printf("Hello, this is chatbot %s. Please type:\n", myName);
        while (1) {
            // Get a string from std input and save it to msgBuf
            char msgBuf[MAX_MSG_LEN];
            gets1(msgBuf);

            printf("I heard you said: %s\n", msgBuf);

            if (strcmp(msgBuf, ":CHANGE") == 0 || strcmp(msgBuf, ":change") == 0) {
                while (1) {
                    printf("Which bot would you like to chat with? ");
                    char newBot[MAX_MSG_LEN];
                    gets1(newBot);

                    // Find the index of the new bot (successor)
                    int newBotIndex = -1;
                    for (int i = 0; i < botCount; i++) {
                        if (strcmp(newBot, botNames[i]) == 0) {
                            newBotIndex = i;
                            break;
                        }
                    }

                    // If bot exists and is not the current bot, switch
                    if (newBotIndex != -1 && newBotIndex != myId - 1) {
                        strcat(msgBuf, "!");
                        strcat(msgBuf, newBot);
                        printf("Okay, I will send you to chat with %s\n", newBot); // Debug: Print message to be sent
                        write(fd[myId][1], msgBuf, MAX_MSG_LEN);
                        exit(0);
                    }
                    // If bot exists and is the current bot, inform the user and leave the loop
                    else if (newBotIndex == myId - 1) {
                        printf("Great, you chose to stay. Please continue typing: \n");
                        break;
                    }
                    else {
                        printf("Bot %s does not exist. Please try again.\n", newBot);
                    }
                }
            }
            // If user inputs :EXIT, exit the chatbot
            else if (strcmp(msgBuf, ":EXIT") == 0 || strcmp(msgBuf, ":exit") == 0) {
                write(fd[myId][1], msgBuf, MAX_MSG_LEN);
                exit(0);
            } else {
                printf("Please continue typing:\n");
            }
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
    // Add the current bot name to the string
    strcat(startMsg, argv[1]);
    // // Increment the current bot index for the next message cycle
    // currentbot = (currentbot + 1) % botCount;
    // if (currentbot == 0) {
    //     currentbot = 1; // Reset to 1 to avoid argv[0]
    // }

    // Get length of the whole string
    int len = strlen(startMsg);

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
