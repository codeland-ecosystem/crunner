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

int main(){
	printf("crunner v4!\n");
	int create_socket, new_socket;
	socklen_t addrlen;
	int port = atoi( (getenv("runnerPort") != NULL) ? getenv("runnerPort") : "15000" );
	struct sockaddr_in address;

	if ((create_socket = socket(AF_INET, SOCK_STREAM, 0)) > 0){
		printf("The socket was created\n");
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	if (bind(create_socket, (struct sockaddr *) &address, sizeof(address)) == 0){
		printf("Binding Socket on port %i\n", port);
	} else {
		printf("%s\n", "Can not bind to socket!");
		exit(1);
	}

	// loop waiting for client to connect
	while (1){
		// get new memory
		int buffer_size = 2048000;
		char *buffer = malloc(buffer_size);
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

		int numbytes;
		int content_length = 0;
		char end[4];
		int parse_passed = 1;
		char *bufferPosition = buffer;
		int foundBody = 0;
		char *body;
		while(1) { //client receiving code
			printf("while...\n");
			if((numbytes = recv(new_socket, bufferPosition, buffer_size, 0)) == -1){
				printf("recv error: %d", numbytes);
				exit(1);
			}

			bufferPosition += numbytes;

			if(content_length == 0){
				char* pcontent = strstr((char*)buffer,"Content-Length:");
				// get the length of the data
				content_length = atoi(pcontent+15);
			}

			if(content_length > buffer_size){
				printf("buffer overflow\n");
				parse_passed = 0;
				break;
			}

			// parse body out of request
			char *bodyStart = strstr(buffer,"\r\n\r\n");
			body = &bodyStart[4];
			int body_length = strlen(body);
			
			printf("recived bytes is %d, full content length is %d body is %d\n", numbytes, content_length, strlen(body));

			if(content_length == 0){
				printf("zero length POST\n");
				parse_passed = 0;
				break;
			}


			if(body && body_length > content_length){
				printf("body over flow\n");
				parse_passed = 0;
				break;
			}

			if(body && body_length == content_length){
				printf("break\n");
				break;
			}else{
				printf("%d continue\n", body_length);
				continue;
			}

			printf("should not get here...\n");
		}

		// if there are errors in parsing, kill the socket.
		if(parse_passed == 0){
			printf("error in request\n");
			close(new_socket);
			continue;
		}

		// parse code from the json
		cJSON *root = cJSON_Parse(body);
		// printf("body %c:\n%s\n", body[0], body);

		char *code = cJSON_GetObjectItem(root, "code")->valuestring;

		// set up code string for POPEN
		char codeToRun[strlen(code)+15];
		strcpy(codeToRun, code);
		strcat(codeToRun, " 2>&1|base64");

		// set up POPEN
		FILE *fp;
		int status;
		if (!(fp = popen(codeToRun, "r"))){
			printf("DIED");
		}

		// build string from popen return
		char *line = NULL, *tmp = NULL;
		size_t size = 0, index = 0;
		int ch = EOF;

		while (ch) {
			ch = getc(fp);
			/* Check if we need to stop. */
			if (ch == EOF){
					ch = 0;
			}
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
			if(ch == '\n'){
				continue;
			}
			// Actually store the thing.
			line[index++] = ch;
		}

		// clear buffer for next request
		fflush(fp);
		printf("\npopen code: %d\n",pclose(fp));

		// Format JSON response
		char rjson[strlen(line)+20];
		strcpy(rjson,"{\"res\":\"");
		strcat(rjson, line);
		strcat(rjson,"\"}\0");
		
		int rjsonLength = strlen(rjson)+0;
		char rjsonLength_char[20];
		sprintf(rjsonLength_char, "%d", rjsonLength);
		
		// format headers and add body
		char rex[rjsonLength+105];
		strcpy(rex, "HTTP/1.0 200 OK\r\n");
		strcat(rex, "Content-Type: application/json\r\n");
		strcat(rex, "Content-Length: ");
		strcat(rex, rjsonLength_char);
		strcat(rex, "\r\n\r\n");
		strcat(rex, rjson);

		// send response to client
		write(new_socket, rex, strlen(rex));    
		close(new_socket);
		free(buffer);
	}
	close(create_socket);    
	return 0;    
}
