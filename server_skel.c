#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

int thread_stat[1024];

typedef struct _query {
    int user;
    int action;
    int data;
} query;

typedef struct argm{
    int connfd;
    int threadidx;
} argm;

void *query_hdlr(void *arg);

int main(int argc, char* argv[])
{
    if(argc < 2)
    {
        fprintf(stderr, "Please Enter Port Number\n");
        exit(1);
    }
    int serverSocket = socket(PF_INET, SOCK_STREAM, 0);
    int connfd, caddrlen;
    struct sockaddr_in serverAddr, caddr;
    
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(atoi(argv[1]));
    serverAddr.sin_addr.s_addr = htons(INADDR_ANY);
    
    if(bind(serverSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) == -1)
	    return 0;

    if(listen(serverSocket, 1024) < 0)
	    return 0;

    /*
     * Insert your PA3 server code
     *
     * You should generate thread when new client accept occurs
     * and process query of client until get termination query
     *
     */
    caddrlen = sizeof(caddr);
    pthread_t *cthreads = (pthread_t *)calloc(1024, sizeof(pthread_t));

    while(1)
    {
        int num_client = 0;
        for(int i = 0; i < 1024; i++) // How many active clients
        {
            if(thread_stat[i])
                num_client++;
        }

        if(num_client < 1024)
        {
            argm *argm = (struct argm *)malloc(sizeof(argm));
            if ((connfd = accept(serverSocket, (struct sockaddr *)&caddr,(socklen_t *)&caddrlen)) < 0)
            {   
                printf ("accept() failed.\n");
                continue;
            }

            for(int i = 0; i < 1024; i++)
            {
                if(thread_stat[i] == 0)
                {
                    argm->connfd = connfd;
                    argm->threadidx = i;
                    pthread_create(&cthreads[i], NULL, query_hdlr, (void *)argm);
                    thread_stat[i] = 1;
                    break;
                }
            }
        }
    }

    free(cthreads);
    pthread_join(cthreads[0], NULL);
    return 0;
}

void *query_hdlr(void *arg)
{
    argm *argm = (struct argm *)arg;
    int connfd = argm->connfd;
    int threadidx = argm->threadidx;
    free(argm);

    query query;
    int return_val = 1;

    while(1)
    {
        if(recv(connfd, &query, sizeof(query), 0) < 0)
        {
            fprintf(stderr, "Recieve Failed\n");
            close(connfd);
            thread_stat[threadidx] = 0;
            return NULL;
        }

        printf("%d %d %d\n", query.user, query.action, query.data);

        if(query.user == 0 && query.action == 0 && query.data == 0)
            return_val = 256;
        
        if(send(connfd, (int *)&return_val, sizeof(return_val), 0) < 0)
        {
            fprintf(stderr, "Send Failed\n");
            close(connfd);
            thread_stat[threadidx] = 0;
            return NULL;
        }

        if(return_val == 256)
            break;
    }

    close(connfd);
    thread_stat[threadidx] = 0;
    return NULL;
}