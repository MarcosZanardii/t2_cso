#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_LENGTH 256

int main()
{
	int ret, fd, len;
	char receive[BUFFER_LENGTH];
	char stringToSend[BUFFER_LENGTH];
	
	printf("Starting device test code example...\n");
	
	fd = open("/dev/pubsub", O_RDWR);
	if (fd < 0) {
		perror("Failed to open the device...");
		return errno;
	}
	
	while (1) {
		printf("ENTER WITH SOME OF OPTIONS:	/subscribe	/unsubscribe	/publish	/fetch\n");

		memset(stringToSend, 0, BUFFER_LENGTH);
		fgets(stringToSend, BUFFER_LENGTH - 1, stdin);
		len = strnlen(stringToSend, BUFFER_LENGTH);
		stringToSend[len - 1] = '\0';
		
		if (len == 1) break;

		ret = write(fd, stringToSend, strlen(stringToSend));
		if (ret < 0) {
			perror("Failed to write the message to the device.");
			return errno;
		}
	}

	printf("Press ENTER to read back from the device...\n");
	getchar();
	
	while (1) {
		memset(receive, 0, BUFFER_LENGTH);
		ret = read(fd, receive, BUFFER_LENGTH);
		if (ret < 0) {
			perror("Failed to read the message from the device.");
			return errno;
		}
		
		len = strnlen(receive, BUFFER_LENGTH);
		if (len == 0) break;
		printf("Read message: [%s]\n", receive);
	}
	
	printf("End of the program\n");
	
	return 0;
}
