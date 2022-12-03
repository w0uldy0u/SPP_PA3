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

int make_query(query *query, FILE *fp);
int print_result(query *query, int return_val, int client_socket);

int main (int argc, char *argv[])
{
    int client_socket = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    struct hostent *h;
    FILE* fp = NULL;

    if (argc == 3){
    } else if (argc == 4) {
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
        if(make_query(&query, fp) < 0)
            continue;

        if(send(client_socket, (struct query *)&query, sizeof(query), 0) < 0)
        {
            fprintf(stderr, "Send Failed\n");
            exit(1);
        }

        if(recv(client_socket, &return_val, sizeof(return_val), 0) < 0)
        {
            fprintf(stderr, "Receive Failed\n");
            exit(1);
        }

        if(print_result(&query, return_val, client_socket) < 0)
            break;
    }

    close(client_socket);
    if(fp != NULL)
        fclose(fp);

    return 0;
}

int make_query(query *query, FILE * fp)
{
    char *buf = NULL;
    size_t size = 0;
    if(fp == NULL)
        getline(&buf, &size, stdin);

    else
    {
        if(getline(&buf, &size, fp) < 0)
        {
            query->user = 0;
            query->action = 0;
            query->data = 0;
            free(buf);
            buf = NULL;
        }
    }

    /* Parse query from buf*/
    if(buf != NULL)
    {
        int idx = 0;
	    char *ptr = strtok(buf, " ");
	    while(ptr != NULL)
	    {
            switch(idx)
            {
                case 0:
                    query->user = atoi(ptr);
                    break;
                case 1:
                    query->action = atoi(ptr);
                    break;
                case 2:
                    query->data = atoi(ptr);
                    break;
            }
            
            for(int i = 0; ptr[i] != '\0'; i++)
            {
                if(ptr[i] < 48 || ptr[i] > 57 )
                {
                    idx = 3;
                    break;
                }
            }

            if(idx == 1)
                ptr = strtok(NULL, "\n");
            else
		        ptr = strtok(NULL, " ");
            idx++;
	    }

        free(buf);
        if(idx != 3)
        {
            printf("Invalid query\n");
            return -1;
        }
    }
    return 0;
}

int print_result(query *query, int return_val, int client_socket)
{
    if((query->user < 0 || query->user > 1023 || query->action < 1 || query->action > 5) && return_val == -1)
        printf("Invalid query\n");

    else if(query->action == 1)   // Login
    {
        if(return_val == 1)
            printf("Login success\n");
        else
            printf("Login failed\n");
    }

    else if(query->action == 2)  // Reservation
    {
        if(return_val == -1)
        {
            if(query->data < 0 || query->data > 255)
                printf("Invalid query\n");
            else
                printf("Reservation failed\n");
        }
        else
            printf("Seat %d is reserved\n", return_val);
    }

    else if(query->action == 3)  // Check Reservation
    {
        if(return_val == 1)
        {
            int seats[256];
            if(recv(client_socket, seats, sizeof(seats), 0) < 0)
            {
                fprintf(stderr, "Receive Failed\n");
                exit(1);
            }

            printf("Reservation:");
            int flag = 0;
            for(int i = 0; i < 256; i++)
            {
                if(seats[i] == 1)
                {
                    if(flag == 0)
                    {
                        printf(" %d", i);
                        flag = 1;
                    }
                    
                    else
                        printf(", %d", i);
                }
            }
            printf("\n");
        }
        else
            printf("Reservation check failed\n");
    }

    else if(query->action == 4)  // Cancel Reservation
    {
        if(return_val == -1)
        {
            if(query->data < 0 || query->data > 255)
                printf("Invalid query\n");
            else
                printf("Cancel failed\n");
        }
        else
            printf("Seat %d is canceled\n", return_val);
    }

    else if(query->action == 5)   // Logout
    {
        if(return_val == 1)
            printf("Logout success\n");
        else
            printf("Logout failed\n");
    }

    else if(return_val == 256)  // Terminate
        return -1;

    return 0;
}