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
	
    /*
     * Insert your PA3 client code
     *
     * You should handle input query
     *
     * Follow the print format below
     *
     * 1. Log in
     * - On success
     *   printf("Login success\n");
     * - On fail
     *   printf("Login failed\n");
     *
     * 2. Reserve
     * - On success
     *   printf("Seat %d is reserved\n");
     * - On fail
     *   printf("Reservation failed\n");
     *
     * 3. Check reservation
     * - On success
     *   printf("Reservation: %s\n");
     *   or
     *   printf("Reservation: ");
     *   printf("%d, ");
     *   ...
     *   printf("%d\n");
     * - On fail
     *   printf("Reservation check failed\n");
     *
     * 4. Cancel reservation
     * - On success
     *   printf("Seat %d is canceled\n");
     * - On fail
     *   printf("Cancel failed\n");
     *
     * 5. Log out
     * - On success
     *   printf("Logout success\n");
     * - On fail
     *   printf("Logout failed\n");
     *
     */
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

        if(return_val == 256)
            break;
    }


    close(client_socket);

    return 0;
}
