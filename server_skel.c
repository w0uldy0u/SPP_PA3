#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

int thread_stat[1024];
int login_user[1024];
int passwd[1024];
int seats[256];

pthread_mutex_t login_m[1024];
pthread_mutex_t seat_m[256];

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
int login(query query, int thread_idx);
int reserve(query query, int thread_idx);
int *chk_reserve(query query, int thread_idx);
int cancel_reserve(query query, int thread_idx);
int logout(query query, int thread_idx);

int main(int argc, char* argv[])
{
    /* Initialize */
    for(int i = 0; i < 1024; i++)
    {
        login_user[i] = -1;
        passwd[i] = -1;

        pthread_mutex_init(&login_m[i], NULL);

        if(i < 256)
        {
            seats[i] = -1;
            pthread_mutex_init(&seat_m[i], NULL);
        }
    }

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
    {
        fprintf(stderr, "Bind Failed\n");
	    exit(1);
    }

    if(listen(serverSocket, 1024) < 0)
    {
	    fprintf(stderr, "Listen Failed\n");
        exit(1);
    }

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
    return 0;
}

void *query_hdlr(void *arg)
{
    argm *argm = (struct argm *)arg;
    int connfd = argm->connfd;
    int threadidx = argm->threadidx;
    free(argm);

    while(1)
    {
        query query;
        int return_val = 1;
        int *chk_seat = NULL;

        if(recv(connfd, &query, sizeof(query), 0) < 0)
        {
            fprintf(stderr, "Receive Failed\n");
            close(connfd);
            thread_stat[threadidx] = 0;
            return NULL;
        }

        if(query.user == 0 && query.action == 0 && query.data == 0)
            return_val = 256;
        
        switch(query.action)
        {
            case 1:
                return_val = login(query, threadidx);
                break;
            case 2:
                return_val = reserve(query, threadidx);
                break;
            case 3:
                chk_seat = chk_reserve(query, threadidx);
                if(chk_seat == NULL)
                    return_val = -1;
                break;
            case 4:
                return_val = cancel_reserve(query, threadidx);
                break;
            case 5:
                return_val = logout(query, threadidx);
                break;
        }
        
        if(send(connfd, (int *)&return_val, sizeof(return_val), 0) < 0)
        {
            fprintf(stderr, "Send Failed\n");
            close(connfd);
            thread_stat[threadidx] = 0;
            return NULL;
        }

        if(query.action == 3 && return_val == 1)
        {
            if(send(connfd, (int *)chk_seat, 256 * sizeof(int), 0) < 0)
            {
                fprintf(stderr, "Send Failed\n");
                close(connfd);
                thread_stat[threadidx] = 0;
                free(chk_seat);
                return NULL;
            }
            free(chk_seat);
        }

        if(return_val == 256)
        {
            for(int i = 0; i < 1024; i++)
            {
                if(login_user[i] == threadidx)
                {
                    login_user[i] = -1;
                    break;
                }
            }
            break;
        }
    }

    printf("thread exit\n");
    close(connfd);
    thread_stat[threadidx] = 0;
    return NULL;
}

int login(query query, int thread_idx)
{
    pthread_mutex_lock(&login_m[query.user]);   // Lock mutex
    if(login_user[query.user] == -1)
    {
        if(passwd[query.user] == query.data)    // Password correct
        {
            login_user[query.user] = thread_idx;
            pthread_mutex_unlock(&login_m[query.user]);
            return 1;
        }
        
        else if(passwd[query.user] == -1)   // Register
        {
            passwd[query.user] = query.data;
            login_user[query.user] = thread_idx;
            pthread_mutex_unlock(&login_m[query.user]);
            return 1;
        }
    }
    pthread_mutex_unlock(&login_m[query.user]);
    return -1;
}

int reserve(query query, int thread_idx)
{
    if(login_user[query.user] == thread_idx)
    {
        if(query.data >= 0 && query.data < 256)
        {
            pthread_mutex_lock(&seat_m[query.data]);
            if(seats[query.data] == -1)
            {
                seats[query.data] = query.user;
                pthread_mutex_unlock(&seat_m[query.data]);
                return query.data;
            }
            pthread_mutex_unlock(&seat_m[query.data]);
        }
    }

    return -1;
}

int *chk_reserve(query query, int thread_idx)
{
    if(login_user[query.user] == thread_idx)
    {
        int *chk_seat = (int *)calloc(256, sizeof(int));
        int cnt = 0;

        for(int i = 0; i < 256; i++)
        {
            if(seats[i] == query.user)
            {
                chk_seat[i] = 1;
                cnt++;
            }
        }

        if(cnt > 0)
            return chk_seat;

        free(chk_seat);
    }
    
    return NULL;
}

int cancel_reserve(query query, int thread_idx)
{
    if(login_user[query.user] == thread_idx && query.data >= 0 && query.data < 256)
    {
        if(seats[query.data] == query.user)
        {
            seats[query.data] = -1;
            return query.data;
        }
    }

    return -1;
}

int logout(query query, int thread_idx)
{
    if(login_user[query.user] == thread_idx)
    {
        login_user[query.user] = -1;
        return 1;
    }

    return -1;
}