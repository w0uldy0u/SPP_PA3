#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

typedef struct _query {
    int user;
    int action;
    int data;
} query;

int main (int argc, char *argv[])
{
    int client_socket = socket(PF_INET, SOCK_STREAM, 0);
    int cmdin = 0;
    struct sockaddr_in server_addr;
    struct hostent *h;
    FILE* fp;

    if (argc == 3) {
	    /* Insert your code for terminal input */
        cmdin = 1;
        } else if (argc == 4) {
	    /* Insert your code for file input */
        fp = fopen(argv[3], "r");
        if(fp == NULL)
        {
            fprintf(stderr, "Input file not found\n");
            exit(1);
        }
    } else {
	printf("Follow the input rule below\n");
	printf("==================================================================\n");
	printf("argv[1]: server address, argv[2]: port number\n");
	printf("or\n");
	printf("argv[1]: server address, argv[2]: port number, argv[3]: input file\n");
	printf("==================================================================\n");
	exit(1);
    }

    char *host = argv[1];
    if ((h = gethostbyname(host)) == NULL) {
        printf("invalid hostname %s\n", host); 
        exit(2);
    }

    memset((char *)&server_addr, 0, sizeof(server_addr));
    memcpy((char *)&server_addr.sin_addr.s_addr, (char *)h->h_addr, h->h_length);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
	printf("Connection failed\n");
	exit(1);
    }
	
    query query;
    int return_val;
    while(1)
    {
        if(cmdin == 1)
            scanf("%d %d %d", &query.user, &query.action, &query.data);

        else
        {
            if(fscanf(fp, "%d %d %d", &query.user, &query.action, &query.data) == EOF)
            {
                query.user = 0;
                query.action = 0;
                query.data = 0;
            }
        }

        if(send(client_socket, (struct query *)&query, sizeof(query), 0) < 0)
        {
            fprintf(stderr, "Send Failed\n");
            exit(1);
        }

        if(recv(client_socket, &return_val, sizeof(return_val), 0) < 0)
        {
            fprintf(stderr, "Recieve Failed\n");
            exit(1);
        }

        printf("Return Val: %d\n", return_val);

        if(query.action == 1)   // Login
        {
            if(return_val == 1)
                printf("Login success\n");
            else
                printf("Login failed\n");
        }

        else if(query.action == 2)  // Reservation
        {
            if(return_val == -1)
                printf("Reservation failed\n");
            else
                printf("Seat %d is reserved\n", return_val);
        }

        else if(query.action == 3)  // Check Reservation
        {
            if(return_val == 1)
            {
                int seats[256];
                if(recv(client_socket, seats, sizeof(seats), 0) < 0)
                {
                    fprintf(stderr, "Recieve Failed\n");
                    exit(1);
                }

                printf("Reservation:");
                for(int i = 0; i < 256; i++)
                {
                    if(seats[i] == 1)
                        printf(" %d", i);
                }
                printf("\n");
            }
            else
                printf("Reservation check failed\n");
        }

        else if(query.action == 4)  // Cancel Reservation
        {
            if(return_val == -1)
                printf("Cancel failed\n");
            else
                printf("Seat %d is canceled\n", return_val);
        }

        else if(query.action == 5)   // Logout
        {
            if(return_val == 1)
                printf("Logout success\n");
            else
                printf("Logout failed\n");
        }

        else if(return_val == 256)  // Terminate
            break;
    }

    close(client_socket);
    fclose(fp);
    return 0;
}