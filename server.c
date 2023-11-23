#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#define SERVER_PORT 5432
#define MAX_PENDING 5
#define MAX_LINE 256
int main() {
    struct sockaddr_in sin, client;
    char buf[MAX_LINE];
    int len;
    int s, new_s;
    time_t t;
    struct tm tm;
    char str[MAX_LINE];
    /* build address data structure */
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(SERVER_PORT);
    /* setup passive open */
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("simplex-talk: socket");
        exit(1);
    }
    if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0) {
        perror("simplex-talk: bind");
        exit(1);
    }

    listen(s, MAX_PENDING);
    /* wait for connection, then receive and print text */
    
    while(1) {
        if ((new_s = accept(s, (struct sockaddr *)&sin, &len)) < 0) {
            perror("simplex-talk: accept");
            exit(1);
        }
        while (len = recv(new_s, buf, sizeof(buf), 0)) {
            //print out the info received from client
            fputs(buf, stdout);
            
            //set the time
            t = time(NULL);
            tm = *localtime(&t);
            sprintf(str, "server received time: %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

            //concat the time string with the received info
            strcat(buf, str);
            //send the new info to client
            send(new_s, buf, MAX_LINE, 0);
            //if received "Ciao-Ciao", end the connection (break out the infinit loop, then end)
            if (strncmp("Ciao-Ciao", buf, 9) == 0) {
                fprintf(stdout, "End the connection...\n");
                break;
            }
        }
        
        //close the socket
        close(new_s);
    }
}