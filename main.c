//Uses Unix sockets (No WinSock!)

#include "server.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>

//Used http://www.pcs.cnu.edu/~dgame/sockets/server.c and https://www.gnu.org/software/libc/manual/html_node/Sockets.html for ideas to start
int main()
{
    int addrlen, sd;
    struct sockaddr_in sockaddr;
    struct sockaddr_in pin;

    while(1)
    {
        if((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            perror("socket");
            exit(1);
        }

        name.sin_family = AF_INET;
        name.sin_port = htons(PORT);
        name.sin_addr-s.ddr = htonl(INADDR_ANY);

        if(bind(sd, (struct sockaddr_in*) &sockaddr, sizeof(sockaddr)) < 0)
        {
            perror("bind");
            exit(1);
        }

        //Wait for message on socket

        //Spin off into it's own thread to listen
    }


}
