#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_LENGTH 256
#define MAX_MSG_SIZE 250

int main()
{
    int ret, fd;
    char commandToSend[BUFFER_LENGTH];
    char messageBuffer[MAX_MSG_SIZE];
    ssize_t bytes_read;
    
    printf("Starting PubSub user-space client...\n");
    
    fd = open("/dev/pubsub_driver", O_RDWR);
    if (fd < 0) {
        perror("Failed to open the device /dev/pubsub_driver");
        return errno;
    }
    
    while (1) {
        printf("\nEnter command (/subscribe, /publish, /fetch, /unsubscribe) or press ENTER to exit:\n> ");
        
        memset(commandToSend, 0, BUFFER_LENGTH);
        
        if (fgets(commandToSend, BUFFER_LENGTH - 1, stdin) == NULL) {
            break;
        }
        
        commandToSend[strcspn(commandToSend, "\n")] = 0;
    
        if (strlen(commandToSend) == 0) {
            break;
        }

        if (strncmp(commandToSend, "/fetch", 6) == 0) {
            ret = write(fd, commandToSend, strlen(commandToSend));
            if (ret < 0) {
                perror("Failed to write /fetch command to the device");
                continue;
            }

            printf("--- Fetching messages ---\n");
            
            while ((bytes_read = read(fd, messageBuffer, sizeof(messageBuffer) - 1)) > 0) {
                messageBuffer[bytes_read] = '\0';
                printf("  [MSG]: %s\n", messageBuffer);
            }

            if (bytes_read == 0) {
                printf("--- End of messages ---\n");
                perror("An error occurred while reading messages");
            }
            
        } else {
            ret = write(fd, commandToSend, strlen(commandToSend));
            if (ret < 0) {
                perror("Failed to write to the device");
            }
        }
    }

    close(fd);
    printf("\nExiting PubSub client.\n");
    
    return 0;
}