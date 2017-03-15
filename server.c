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
	int port = atoi( (getenv("runnerPort") != NULL) ? getenv("runnerPort") : "15001" );
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
		int bufsize = 2048000;
		char *buffer = malloc(bufsize);
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
		int recvlen = 0;
		int conlen = 0;
		char end[4];
		int parsePassed = 1;
		char *bufferPosition = buffer;
		while(1) { //client receiving code
			printf("while...\n");
	        	if((numbytes = recv(new_socket, bufferPosition, bufsize, 0)) == -1){
	            		printf("recv error: %d", numbytes);
	            		exit(1);
	        	}

	        	bufferPosition += numbytes;
                        end[0] = buffer[numbytes-4];
                        end[1] = buffer[numbytes-3];
                        end[2] = buffer[numbytes-2];
                        end[3] = buffer[numbytes-1];

	        	if(conlen == 0){
	        		char* pcontent = strstr((char*)buffer,"Content-Length:");
	        	    	// get the length of the data
	        		conlen = atoi(pcontent+15);
	        	}

	        	// buffer[numbytes] = '\0';
	        	printf("recived bytes is %d, full content length is %d, buffersize is %d\n", numbytes, conlen, bufferPosition);
			printf("end # %d, %d, %d, %d\n", end[0], end[1], end[2], end[3]);
	        	// printf("end is: %s\n", end);
	        	// printf("last char, %d, %d, %d\n", buffer[numbytes-1], buffer[numbytes], buffer[numbytes+1]);
	        	printf("received:\n%s\n", buffer);

			// printf("checking len\n");
        		if(conlen == 0){
        			printf("zero length POST\n");
        			parsePassed = 0;
        			break;
        		}

			// printf("checking end\n");
        		if(strncmp(end, "\r\n\r\n", 4) == 0){
        			printf("end is 2 new lines, getting more\n");
        			continue;
        		}

			if(numbytes >= conlen){
				printf("seemd to be done\n");
				break;
			}

			// printf("checking numbytes and len\n");
	        	if(numbytes == 0 || numbytes >= conlen){
	        		printf("done with message\n");
	        		break;
	        	}
	        	printf("should not get here...");
	    }

	    if(parsePassed == 0 || strncmp(buffer, "\r\n\r\n", 4) == 0){
	    	printf("error in request");
	    	close(new_socket);
	    	continue;
	    }

	    // printf("buffer:\n%s", buffer);
		// printf("buffer before:\n%s", buffer);
		// read content from socket
		// ssize_t n;
		// char *p = buffer;
		// while( (n = recv(new_socket, p, bufsize, 0)) > 0){
		// 	p += n;
		//	printf("buffer %zd loop:\n%s", n, p);
		//	printf("buffer loop2:\n%s", buffer);
		//	bufsize =- (size_t)n;
		//	// printf("n? %zd", n);
		// }
		// int bufsize = 2048;
		// char *headers = malloc(bufsize);
		// size_t hlen = recv(new_socket, headers, bufsize, 0);
		// printf("headers %zd:\n%s", hlen, headers);

		// int bufsize2 = 2048000;
		// char *body = malloc(bufsize2);
		// size_t blen = recv(new_socket, body, bufsize2, 0);
		// printf("body %zd:\n%s", blen, body);
		// tell the client to wait for a response
		// write(new_socket, "HTTP/1.0 100 Continue\r\n", 28);

		// parse body out of request
		char *body = strstr(buffer,"\r\n\r\n");

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
