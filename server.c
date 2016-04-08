#include <stdio.h>  
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
	
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "cJSON/cJSON.h"

// function to split strings
#define MX_SPLIT 128
int split( char **result, char *working, const char *src, const char *delim){
	int i;

	strcpy(working, src); // working will get chppped up instead of src 
	char *p=strtok(working, delim);
	for(i=0; p!=NULL && i < (MX_SPLIT -1); i++, p=strtok(NULL, delim) ){
		result[i]=p;
		result[i+1]=NULL;  // mark the end of result array
	}

	return i;
}

int main(){
	int create_socket, new_socket;
	socklen_t addrlen;
	int bufsize = 2048000;
	char *buffer = malloc(bufsize);
	struct sockaddr_in address;
	if ((create_socket = socket(AF_INET, SOCK_STREAM, 0)) > 0){
		printf("The socket was created\n");
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(15000);

	if (bind(create_socket, (struct sockaddr *) &address, sizeof(address)) == 0){
		printf("Binding Socket\n");
	} else {
		printf("%s\n", "Can not bind to socket!");
	}

	// loop waiting for client to connect
	while (1){
		if (listen(create_socket, 10) < 0) {
			perror("server: listen");
			exit(1);
		}

		if ((new_socket = accept(create_socket, (struct sockaddr *) &address, &addrlen)) < 0) {
			perror("server: accept");
			exit(1);
		}

		if (new_socket > 0){
			printf("The Client is connected...\n");
		}

		// parse body out of request
		recv(new_socket, buffer, bufsize, 0);
		char *result[MX_SPLIT]={NULL};
		char working[500]={0x0};
		char mydelim[]="\n\n";
		int splitLen;

		splitLen = split(result, working, buffer, mydelim);

		// parse JSON
		char *json=result[splitLen-1];
		cJSON * root = cJSON_Parse(json);
		char * code = cJSON_GetObjectItem(root, "code")->valuestring;

		// set up code string for POPEN
		char codeToRun[strlen(code)+41];
		strcpy(codeToRun, "echo \"");
		strcat(codeToRun, code);
		strcat(codeToRun, "\"|base64 --decode| bash | base64");

		printf("%s\n", codeToRun);

		// set up POPEN
		FILE *fp;
		int status;
		if (!(fp = popen(codeToRun, "r"))){
				printf("DIED");
		}

		char *line = NULL, *tmp = NULL;
		size_t size = 0, index = 0;
		int ch = EOF;

		while (ch) {
			ch = getc(fp);
			/* Check if we need to stop. */
			if (ch == EOF)
					ch = 0;

			/* Check if we need to expand. */
			if (size <= index) {
				size += 50;
				tmp = realloc(line, size);
				if (!tmp) {
					free(line);
					line = NULL;
					break;
				}
				line = tmp;
			}

			/* Actually store the thing. */
			line[index++] = ch;
		}

		// we should check this
		// status = pclose(fp);

		// Format JSON response
		char rex[strlen(line)+11];
		strcpy(rex,"{\"res\":\"");
		strcat(rex, line);
		strcat(rex,"\"}");

		// send response to client
		write(new_socket, rex, strlen(rex));    
		close(new_socket);
	}

	close(create_socket);    
	return 0;    
}
