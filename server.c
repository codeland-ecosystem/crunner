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

#define MX_SPLIT 128




// ******************************************************************************************
// http://knol2share.blogspot.com/2011/07/base64-encoding-and-decoding-in-c.html
// ******************************************************************************************
/* Macro definitions */
#define TABLELEN        63
#define BUFFFERLEN      128

#define ENCODERLEN      4
#define ENCODEROPLEN    0
#define ENCODERBLOCKLEN 3

#define PADDINGCHAR     '='
#define BASE64CHARSET   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"\
                        "abcdefghijklmnopqrstuvwxyz"\
                        "0123456789"\
                        "+/";
/* Function prototypes */
int Base64Encode(char *input, char *output, int oplen);
int encodeblock(char *input, char *output, int oplen);
int Base64Decode(char *input, char *output, int oplen);
int decodeblock(char *input, char *output, int oplen);
/* Its always better to move the macros and function prototypes to a header file */

int decodeblock(char *input, char *output, int oplen){
   int rc = 0;
   char decodedstr[ENCODERLEN + 1] = "";

   decodedstr[0] = input[0] << 2 | input[1] >> 4;
   decodedstr[1] = input[1] << 4 | input[2] >> 2;
   decodedstr[2] = input[2] << 6 | input[3] >> 0;
   strncat(output, decodedstr, oplen-strlen(output));

   return rc;
}

int Base64Decode(char *input, char *output, int oplen){
   char *charval = 0;
   char decoderinput[ENCODERLEN + 1] = "";
   char encodingtabe[TABLELEN + 1] = BASE64CHARSET;
   int index = 0, asciival = 0, computeval = 0, iplen = 0, rc = 0;

   iplen = strlen(input);
   while(index < iplen){
      asciival = (int)input[index];
      if(asciival == PADDINGCHAR){
         rc = decodeblock(decoderinput, output, oplen);
         break;
      }else{
         charval = strchr(encodingtabe, asciival);
         if(charval){
            decoderinput[computeval] = charval - encodingtabe;
            computeval = (computeval + 1) % 4;
            if(computeval == 0){
               rc = decodeblock(decoderinput, output, oplen);
               decoderinput[0] = decoderinput[1] =
               decoderinput[2] = decoderinput[3] = 0;
            }
         }
      }
      index++;
   }

   return rc;
}

int encodeblock(char *input, char *output, int oplen){
   int rc = 0, iplen = 0;
   char encodedstr[ENCODERLEN + 1] = "";
   char encodingtabe[TABLELEN + 1] = BASE64CHARSET;

   iplen = strlen(input);
   encodedstr[0] = encodingtabe[ input[0] >> 2 ];
   encodedstr[1] = encodingtabe[ ((input[0] & 0x03) << 4) |
                                 ((input[1] & 0xf0) >> 4) ];
   encodedstr[2] = (iplen > 1 ? encodingtabe[ ((input[1] & 0x0f) << 2) |
                                              ((input[2] & 0xc0) >> 6) ] : PADDINGCHAR);
   encodedstr[3] = (iplen > 2 ? encodingtabe[ input[2] & 0x3f ] : PADDINGCHAR);
   strncat(output, encodedstr, oplen-strlen(output));

   return rc;
}

int Base64Encode(char *input, char *output, int oplen){
   int rc = 0;
   int index = 0, ipindex = 0, iplen = 0;
   char encoderinput[ENCODERBLOCKLEN + 1] = "";

   iplen = strlen(input);
   while(ipindex < iplen){
      for(index = 0; index < 3; index++){
         if(ipindex < iplen){
            encoderinput[index] = input[ipindex];
         }else{
            encoderinput[index] = 0;
         }
         ipindex++;
      }
      rc = encodeblock(encoderinput, output, oplen);
   }

   return rc;
}
















char **split( char **result, char *working, const char *src, const char *delim)
{
    int i;

    strcpy(working, src); // working will get chppped up instead of src 
    char *p=strtok(working, delim);
    for(i=0; p!=NULL && i < (MX_SPLIT -1); i++, p=strtok(NULL, delim) )
    {
        result[i]=p;
        result[i+1]=NULL;  // mark the end of result array
    }
    return result;
}

// void foo(const char *somestring)
// {
//    int i=0;
//    char *result[MX_SPLIT]={NULL};
//    char working[256]={0x0}; // assume somestring is never bigger than 256 - a weak assumption
//    char mydelim[]="!@#$%^&*()_-";
//    split(result, working, somestring, mydelim);
//    while(result[i]!=NULL)
//       printf("token # %d=%s\n", i, result[i]);
// }
// ******************************************************************************************
// http://stackoverflow.com/questions/8164000/how-to-dynamically-allocate-memory-space-for-a-string-and-get-that-string-from-u
// ******************************************************************************************
// char *getln()
// {
//     char *line = NULL, *tmp = NULL;
//     size_t size = 0, index = 0;
//     int ch = EOF;

//     while (ch) {
//         ch = getc(stdin);

//         /* Check if we need to stop. */
//         if (ch == EOF || ch == '\n')
//             ch = 0;

//         /* Check if we need to expand. */
//         if (size <= index) {
//             size += CHUNK;
//             tmp = realloc(line, size);
//             if (!tmp) {
//                 free(line);
//                 line = NULL;
//                 break;
//             }
//             line = tmp;
//         }

//         /* Actually store the thing. */
//         line[index++] = ch;
//     }

//     return line;
// }
int main() {
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

   while(1){
       if (bind(create_socket, (struct sockaddr *) &address, sizeof(address)) == 0){    
          printf("Binding Socket\n");
          break;
       }
       printf("%s\n", "not ready!");
       exit(1);

    }
    
    
   while (1) {    
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
        
      recv(new_socket, buffer, bufsize, 0);    
      char *result[MX_SPLIT]={NULL};
      char working[500]={0x0};
      char mydelim[]="\n\n";

      split(result, working, buffer,mydelim);
      int i=0;
      while(result[i]!=NULL){
        if (result[i+1]==NULL){
            // jsmn_parse(&parser, result[i], strlen(result[i]), tokens, 10);
        }
        i++;
      };
      char *json=result[i-1];
      cJSON * root = cJSON_Parse(json);
      char * code = cJSON_GetObjectItem(root,"code")->valuestring;

    int rc = 0;
     char decodedoutput[BUFFFERLEN + 1] = "";

    rc = Base64Decode(code, decodedoutput, BUFFFERLEN);

    FILE *fp;
    int status;
    char codeToRun[strlen(code)+41];
    
    printf("dsklfjasawioejfioa\n\n");
    strcpy(codeToRun, "echo \"");
    strcat(codeToRun,code);
    strcat(codeToRun, "\"|base64 --decode| bash | base64");
    printf("%s\n",codeToRun);
    if (!(fp = popen(codeToRun, "r"))){
        printf("DIED");
        exit(1);
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



    status = pclose(fp);
    // if (status == -1) {
    //     /* Error reported by pclose() */
    //     ...
    // } else {
    //      Use macros described under wait() to inspect `status' in order
    //        to determine success/failure of command executed by popen() 
    //     ...
    // } 
        char rex[strlen(line)+11];
        strcpy(rex,"{\"res\":\"");
        strcat(rex, line);
        strcat(rex,"\"}");
             
 
        write(new_socket, rex, strlen(rex));    
        close(new_socket);
      // exit(1);
    }    
    close(create_socket);    
   return 0;    
}
