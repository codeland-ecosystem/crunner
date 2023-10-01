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
	printf("crunner v9 -- November 1, 2023!\n");

	int create_socket, new_socket;
	int buffer_size = 2048000;
	socklen_t addrlen;
	int port = atoi( (getenv("runnerPort") != NULL) ? getenv("runnerPort") : "15000" );
	struct sockaddr_in address;

	// Add a flag to indicate whether a client is being served

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


        // Set the flag to indicate that a client is being served

		char *buffer = malloc(buffer_size);
		char *bufferPosition = buffer;
		int bytes_from_socket = 0;
		int content_length = 0;
		int parse_passed = 1;
		char *body = malloc(1);  // Initialize with space for '\0'
		body[0] = '\0';

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

		/*
			loop over the socket until the whole message is received
		*/
		while(1) { //client receiving code

			if((bytes_from_socket = recv(new_socket, bufferPosition, buffer_size, 0)) == -1){
				printf("recv error: %d", bytes_from_socket);
				exit(1);
			}

			// move the end of the buffer
			bufferPosition += bytes_from_socket;

			if(content_length == 0){
				/*
					This should only happen once, after the content length is known
				*/

				// Check to see if this i a POST method
				// All we need to do is check the first character in the buffer
				// (body) and see if its P, ASCII 80
				if(buffer[0] != 80){
					printf("Method is not post: %d\n", buffer[0]);
					parse_passed = 0;
					content_length = 1;
					break;
				}

				// Check for content length in the header
				if (strstr(buffer, "Content-Length:") == NULL) {
					parse_passed = 0;
					break;
				}else{
					char* pcontent = strstr((char*)buffer, "Content-Length:");
					content_length = atoi(pcontent+15);
				}

				// dont write more then the buffer can handle
				if(content_length > buffer_size){
					printf("buffer overflow\n");
					parse_passed = 0;
					break;
				}

				// drop the connection if the content length is zero
				if(content_length == 0){
					printf("zero length\n");
					parse_passed = 0;
					break;
				}
			}



			// Reset and allocate memory for body at the beginning of each loop iteration
		    free(body);
		    body = malloc(content_length + 1);  // Allocate memory for body (+1 for '\0')
		    if (body == NULL) {
		        // Handle memory allocation error
		        perror("Memory allocation error");
		        exit(1);
		    }

		    // Parse body out of request
		    char *bodyStart = strstr(buffer, "\r\n\r\n");
		    if (bodyStart != NULL) {
		        bodyStart += 4;
		        strncpy(body, bodyStart, content_length);
		        body[content_length] = '\0';  // Null-terminate the string
		    }


			int body_length = strlen(body);

			// some debug info
			printf("received bytes is %d, full content length is %d, body is %d\n", bytes_from_socket, content_length, body_length);

			if(body && body_length > content_length){
				printf("body over flow\n");
				// printf("%d, %d\n", body_length, content_length);
				// printf("body:\n%s\n", body);
				parse_passed = 0;
				break;
			}

			if(body && body_length == content_length){
				// printf("received full body\n");
				break;
			}else{
				// printf("read more from buffer");
				continue;
			}

			printf("should not get here...\n");
		}

		// if there are errors in parsing, kill the socket.
		if(parse_passed == 0){
			printf("error in request\n");
			body = "";
			free(buffer);
			close(new_socket);
			continue;
		}



		// Parse code from the JSON
		cJSON *root = cJSON_Parse(body);

		// Check if the "code" key exists in the JSON
		cJSON *existingCode = cJSON_GetObjectItem(root, "code");

		if (existingCode == NULL) {
		    // If it doesn't exist, add "code" as a string
		    cJSON_AddStringToObject(root, "code", "exit 0");
		}

		// Retrieve the "code" value as a string
		char *code = cJSON_GetObjectItem(root, "code")->valuestring;

		// set up code string for POPEN
		char codeToRun[strlen(code)+15];
		strcpy(codeToRun, code);
		strcat(codeToRun, " 2>&1|base64");

		cJSON_Delete(root);

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
		
		free(line);

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
		free(buffer);
		close(new_socket);
	}
	close(create_socket);    
	return 0;    
}
