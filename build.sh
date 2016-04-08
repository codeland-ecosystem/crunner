gcc -c cJSON/cJSON.c server.c
gcc cJSON.o server.o -o s -lm