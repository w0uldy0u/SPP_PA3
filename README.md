<div align = center>
  <H1>
    시스템프로그래밍실습 (SPP_PA3)
  </H1>
  <H3>
    Programming Assignment 3
  </H3>
</div>

<br>
<br>
<br>
<br>
<br>




## 설계 · 구현 내용

PA3는 최대 1024명의 유저와 256개의 좌석을 관리하는 Ticketing server/client를 구현해야 합니다.

Client는 파일 또는 커맨드라인에서 input을 받아 다음과 같은 구조로 된 query를 server로 보내 5가지 기능을 실행합니다.

```c
struct query{
  int user;
  int action;
  int data;
};
```

5가지 기능은 아래와 같습니다.

- **Login (Action 1)**
  - 특정 계정으로 로그인합니다
  - 해당 계정의 첫 로그인일 경우, 회원가입으로 처리합니다
- **Reserve (Action 2)**
  - 특정 좌석을 예약합니다
- **Check reservation (Action 3)**
  - 로그인 된 계정으로 예약된 좌석을 출력합니다
- **Cancel reservation (Action 4)**
  - 로그인 된 계정으로 예약된 특정 좌석의 예약을 취소합니다
- **Logout (Action 5)**
  - 로그인 된 계정에서 로그아웃합니다



Input 예시입니다.

```
1 1 1234
1 2 100
1 4 100
1 4 0
0 0 0
```

첫 번째 숫자는 User, 두번째 숫자는 Action, 세번째 숫자는 Data입니다.



Server는 새로운 client가 연결할때마다 별도의 thread를 만들어 동시에 1024개의 client를 처리할 수 있게합니다.

각 기능에 대한 server의 response code는 다음과 같습니다.

|      ***Action***      | ***On success***      | ***On fail*** |
| :---------------: | :-------------------: | :-------: |
|       Login       | 1                     | -1      |
|      Reserve      | 예약된 좌석 번호          | -1      |
| Check reservation | 예약된 모든 좌석 번호 | -1      |
|      Logout       | 1                     | -1      |

User, Action, Data가 모두 0인 query를 받을 경우 256을 return하고 해당 client의 thread는 종료됩니다.

Server로부터 256을 return받은 client 역시 프로그램을 종료합니다.

<br>

## Client 소스코드
```c
typedef struct _query {	// Server에게 보낼 query 구조체입니다
    int user;
    int action;
    int data;
} query;
```
<br>
<h3> main 함수</h3>

```c
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
```

Argument로 ip주소가 아니라 도메인 네임이 주어져도 작동하도록 gethostbyname() 함수를 활용하였습니다.

```c
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
```

make_query() 함수를 통해 파일 또는 커맨드라인에서 입력을 받고 query 구조체에 형식을 맞춰 저장합니다.

만약 유효하지 않은 input일 경우 make_query()에서 -1을 return받고 다시 make_query()를 실행합니다.

유효한 input을 받았을 경우 query 구조체를 server에게 전송한 뒤 서버의 response를 return_val에 저장합니다.

서버에게 받은 return_val과 query 구조체를 이용해 print_result() 함수가 결과를 출력합니다.

만약 서버에게서 256을 응답 받았다면 print_result는 -1을 return하여 client의 while문을 깨고 종료합니다.

<br>

<h3> make_query 함수</h3>

```c
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
```

파일 포인터가 NULL을 가리키는지에 따라 커맨드라인에서 input을 받을지 파일을 통해 input을 받을지 결정합니다.

파일 또는 커맨드라인에서 getline()을 통해 한 줄씩 읽어들입니다.

만약 파일의 끝(EOF)이라면 query의 각 값에 0, 0, 0을 넣습니다.

```c
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
```

공백을 기준으로 첫 번째 숫자는 user, 두 번째 숫자는 action, 세 번째 숫자는 data에 넣습니다.

getline()이 heap영역에 할당한 공간을 해제합니다.

만약 input이 '숫자공백숫자공백' 형식이 아닐 경우 "Invalid query"라고 출력한 뒤 -1을 return합니다.

정상적으로 query를 만들었을 경우 0을 return합니다.

<br>

<h3>print_result 함수</h3>

```c
    if((query->user < 0 || query->user > 1023 || query->action < 1 || query->action > 5) && return_val == -1)
        printf("Invalid query\n");

    else if(query->action == 1)  
    {
        if(return_val == 1)
            printf("Login success\n");
        else
            printf("Login failed\n");
    }

    else if(query->action == 2) 
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
```

만약 각 데이터의 범위를 벗어난 값을 서버에게 보내 -1을 응답 받았을 경우 "Invalid query"라고 출력합니다.

Login과 reserve 기능 실행 결과를 출력합니다.

```c
    else if(query->action == 3) 
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
```

만약 check reservation 기능을 요청했는데 서버로부터 1을 응답 받았을 경우, 각 자리에 대한 해당 user의 예약 정보를 담은 seats 배열을 수신합니다.

seats배열에서 1로 되어있는 자리가 해당 user가 예약한 자리이므로 해당 자리의 번호를 오름차순으로 모두 출력합니다.

```c
    else if(query->action == 4)  
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

    else if(query->action == 5)  
    {
        if(return_val == 1)
            printf("Logout success\n");
        else
            printf("Logout failed\n");
    }

    else if(return_val == 256) 
        return -1;

    return 0;
```

Cancel reservation과 logout의 실행결과를 출력합니다.

만약 서버로부터 256을 응답받았을 경우 -1을 return하고, 보통의 경우 0을 return합니다.

<br>

## Server 소스코드

```c
int thread_stat[1024];	// 각 thread의 실행 및 종료 여부를 저장하는 배열입니다
int login_user[1024];		// 어떤 User가 어떤 thread를 통해 로그인 중인지 저장하는 배열입니다
int passwd[1024];				// 각 User의 password를 저장하는 배엻입니다
int seats[256];					// 어떤 자리가 어떤 User에 의해 예약되어있는지 저장하는 배열입니다

pthread_mutex_t login_m[1024];	// login_user 배열을 위한 mutex입니다
pthread_mutex_t seat_m[256];		// seats 배열을 위한 mutex입니다
```

```c
typedef struct _query {	// Client에게 받은 query를 위한 구조체입니다
    int user;
    int action;
    int data;
} query;

typedef struct argm{	// Client thread에 넘겨줄 argument를 위한 구조체입니다
    int connfd;
    int threadidx;
} argm;
```
<br>
<h3>main 함수</h3>

```c
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
```

배열과 mutex를 초기화합니다.

로그인하지 않은 user, 아직 설정되지 않은 password, 어떤 user에게도 예약되지 않은 좌석은 -1로 나타냅니다.

```c
    pthread_t *cthreads = (pthread_t *)calloc(1024, sizeof(pthread_t));
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
```

동시에 최대 1024개의 client가 접속할 수 있어야 하므로 1024개의 thread를 만들 준비를 합니다.

각각의 thread가 종료되었을때 자동으로 자원을 회수할 수 있게 detached 속성을 부여합니다.

```c
    while(1)
    {
        int num_client = 0;
        for(int i = 0; i < 1024; i++)
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
                    pthread_create(&cthreads[i], &attr, query_hdlr, (void *)argm);
                    thread_stat[i] = 1;
                    break;
                }
            }
        }
    }
```

thread_stat 배열의 값이 1이면 실행 중인 thread라는 뜻이므로 이를 이용해 현재 thread의 개수를 셉니다.

현재 실행중인 thread의 개수가 1024개 미만일때만 새로운 client를 accept합니다.

새로운 thread를 생성하고 client 소켓의 file descriptor와 해당 thread에 부여한 index를 argument로 넘겨줍니다.

생성된 thread는 query_hdlr() 함수를 실행합니다.

각 thread의 index는 thread의 실행이 종료되면 재사용되어 최대 1023을 넘지 않습니다.

<br>

<h3>query_hdlr 함수</h3>

```c
    argm *argm = (struct argm *)arg;
    int connfd = argm->connfd;
    int threadidx = argm->threadidx;
    free(argm);
```

Argument로 전달받은 구조체에서 connfd와 threadidx를 받고 heap영역에 할당돼 있는 공간을 해제합니다.

```c
    while(1)
    {
        query query;
        int return_val = -1;
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
        
        else if (query.user >= 0 && query.user <= 1023)
        {
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
                    if(chk_seat != NULL)
                        return_val = 1;
                    break;
                case 4:
                    return_val = cancel_reserve(query, threadidx);
                    break;
                case 5:
                    return_val = logout(query, threadidx);
                    break;
            }
        }
```

Client에게서 query를 수신합니다.

만약 user, action, data가 모두 0인 query를 수신했을 경우 256을 반환합니다.

각 action에 맞는 함수를 실행하고 client에 반환할 값을 return 받습니다.

Check reservation의 경우 성공이면 예약 정보를 담은 배열을, 실패면 NULL을 함수로부터 반환 받습니다.

```c
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

    close(connfd);
    thread_stat[threadidx] = 0;
    return NULL;
```

Client에게 반환 값을 전송합니다.

만약 check reservation 기능을 실행해 보내야 할 배열이 있다면 client에게 예약 정보를 담은 배열을 전송하고 배열에 할당된 메모리를 해제합니다.

만약 client에게 256을 반환한다면 로그인 되어있는지 확인 후 로그아웃 시키고 접속을 끊은 뒤 client thread를 종료합니다.

<br>

<h3>login 함수</h3>

```c
    if(query.data < 0)
        return -1;

    pthread_mutex_lock(&login_m[query.user]);   
    if(login_user[query.user] == -1)
    {
        if(passwd[query.user] == query.data)   
        {
            login_user[query.user] = thread_idx;
            pthread_mutex_unlock(&login_m[query.user]);
            return 1;
        }
        
        else if(passwd[query.user] == -1)  
        {
            passwd[query.user] = query.data;
            login_user[query.user] = thread_idx;
            pthread_mutex_unlock(&login_m[query.user]);
            return 1;
        }
    }
    pthread_mutex_unlock(&login_m[query.user]);
    return -1;
```

비밀번호는 음수가 될 수 없기 때문에 음수라면 -1을 return합니다.

동시에 여러 client가 하나의 계정으로 로그인 하는 상황이 있을 수 있기 때문에 mutex를 잡습니다.

로그인 하려는 계정이 다른 client를 통해 로그인 되어있지 않고, 비밀번호가 일치하면 로그인 후 mutex를 풀고 1을 return합니다.

로그인 하려는 계정이 처음 로그인 됐다면 비밀번호를 등록하고 로그인 시킨 후 mutex를 풀고 1을 return합니다.

위의 두가지 상황 외에는 로그인 실패이므로 mutex를 풀고 -1을 return합니다.

<br>

<h3>reserve 함수</h3>

```c
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
```

예약하려는 user가 해당 client를 통해 로그인 되어있고 예약하려는 자리가 비어있다면 해당 자리를 예약합니다.

동시에 여러 client가 같은 자리를 예약하는 상황이 있을 수 있기 때문에 mutex를 잡습니다.

예약에 성공하면 해당 자리를 return하고 실패하면 -1을 return합니다.

<br>

<h3>chk_reserve 함수</h3>

```c
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
```

자리를 확인하려는 user가 해당 client를 통해 로그인 되어있고 해당 user가 예약한 자리가 하나 이상이라면 예약 정보를 담은 배열을 return합니다.

배열에는 해당 user에게 예약된 자리는 1, 아닌 자리는 0을 기록합니다.

위의 상황 외에는 NULL을 return합니다.

<br>

<h3>cancel_reserve 함수</h3>

```c
    if(login_user[query.user] == thread_idx && query.data >= 0 && query.data < 256)
    {
        if(seats[query.data] == query.user)
        {
            seats[query.data] = -1;
            return query.data;
        }
    }

    return -1;
```

예약을 취소하려는 user가 해당 client를 통해 로그인 되어있고 취소하려는 자리가 해당 user에 의해 예약되어 있다면 예약을 취소합니다.

예약 취소에 성공하면 취소한 자리를 return하고 실패하면 -1을 return합니다.

<br>

<h3>logout 함수</h3>

```C
    if(login_user[query.user] == thread_idx)
    {
        login_user[query.user] = -1;
        return 1;
    }

    return -1;
```

로그아웃 하려는 user가 해당 client를 통해 로그인 되어있다면 로그아웃 합니다.

성공하면 1, 실패하면 -1을 return합니다.
