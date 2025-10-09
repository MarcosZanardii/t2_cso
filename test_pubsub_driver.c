#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_LENGTH 256

int main()
{
    int ret, fd;
    char stringToSend[BUFFER_LENGTH];
    
    printf("Starting PubSub user-space client...\n");
    
    fd = open("/dev/pubsub_driver", O_RDWR);
    if (fd < 0) {
        perror("Failed to open the device /dev/pubsub");
        return errno;
    }
    
    while (1) {
        printf("Enter a command (/subscribe <id>, /publish <id> \"message\", /fetch <id>, etc. or press ENTER to exit):\n");
        
        memset(stringToSend, 0, BUFFER_LENGTH);
        
        // Read a line from stdin
        if (fgets(stringToSend, BUFFER_LENGTH - 1, stdin) == NULL) {
            break; // Exit on EOF or error
        }
        
        // Remove the newline character
        stringToSend[strcspn(stringToSend, "\n")] = 0;
        
        // Check if the user wants to exit
        if (strlen(stringToSend) == 0) {
            break;
        }

        // Write the command to the kernel module
        ret = write(fd, stringToSend, strlen(stringToSend));
        if (ret < 0) {
            perror("Failed to write to the device");
            // You might want to handle specific errors differently
        }
    }

    close(fd);
    printf("Exiting PubSub client.\n");
    
    return 0;
}