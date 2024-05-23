#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>

#define BIND_IP_ADDR "127.0.0.1"
#define BIND_PORT 8000
#define MAX_RECV_LEN 1048576
#define MAX_SEND_LEN 1048576
#define MAX_PATH_LEN 1024
#define MAX_HOST_LEN 1024
#define MAX_CONN 1000
#define THREAD_MAX_NUM 100
#define QUEUE_SIZE 1000 

#define HTTP_STATUS_200 "200 OK"
#define HTTP_STATUS_500 "500 Internal Server Error"
#define HTTP_STATUS_404 "404 Not Found"
void handleError(char *error){
    perror(error);
    exit(EXIT_FAILURE);
}
typedef struct{
    pthread_mutex_t lock;
    pthread_cond_t queue_not_full;
    pthread_cond_t queue_not_empty;
    pthread_t *threads;  //����̵߳�tid
    int *tasks_queue;
    int thread_num; // �߳���
    int queue_front;
    int queue_tail;
    int queue_size; // ������
    int queue_max_size; //���������
}threadpool;


int parse_request(char* request, ssize_t req_len, char* path, ssize_t* path_len);
void handle_clnt(int clnt_sock);
void *threadpool_op(void *thread_pool);
threadpool *threadpool_create(int queue_max_size, int thread_num);
void threadpool_add_task(int clnt_sock, threadpool *pool);
int main(){
    // �����׽��֣�����˵����
    //   AF_INET: ʹ�� IPv4
    //   SOCK_STREAM: �������ӵ����ݴ��䷽ʽ
    //   IPPROTO_TCP: ʹ�� TCP Э��

    int serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    threadpool *pool = threadpool_create(QUEUE_SIZE,THREAD_MAX_NUM);
  
    // ���׽��ֺ�ָ���� IP���˿ڰ�
    //   �� 0 ��� serv_addr������һ�� sockaddr_in �ṹ�壩
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    //   ���� IPv4
    //   ���� IP ��ַ
    //   ���ö˿�
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(BIND_IP_ADDR);
    serv_addr.sin_port = htons(BIND_PORT);
    //   ��
    bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    
    // ʹ�� serv_sock �׽��ֽ������״̬����ʼ�ȴ��ͻ��˷�������
    listen(serv_sock, MAX_CONN);
 
    // ���տͻ������󣬻��һ��������ͻ���ͨ�ŵ��µ����ɵ��׽��� clnt_sock
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size = sizeof(clnt_addr);

    while(1){
        // ��û�пͻ�������ʱ��accept() ����������ִ�У�ֱ���пͻ������ӽ���
        int clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
        // ����ͻ��˵�����
        // handle_clnt(clnt_sock);
        if(clnt_sock == -1){
            handleError("accept error\n");
        }
        threadpool_add_task(clnt_sock, pool);
      
    }

    // ʵ��������Ĵ��벻�ɵ�������� while ѭ�����յ� SIGINT �ź�ʱ���� break
    // �ر��׽���
    close(serv_sock);
    return 0;
}
threadpool *threadpool_create(int queue_max_size, int thread_num){
    threadpool *pool = (threadpool *)malloc(sizeof(threadpool));
    if(!pool){
        handleError("threadpool malloc fail\n");
    }
    //��ʼ��
    pool->queue_front = 0;
    pool->queue_size  = 0;
    pool->thread_num = thread_num;
    pool->queue_tail = 0;
    // pool->shutdown = 0;
    pool->queue_max_size = queue_max_size;
    //Ϊ�߳̿�����Ӧ�Ŀռ�
    pool->threads = (pthread_t *)malloc(thread_num * sizeof(pthread_t));
    if(!pool->threads){
        handleError("threads malloc fail\n");
    }
    pool->tasks_queue = (int *)malloc(sizeof(int) * queue_max_size);
    if(!pool->tasks_queue){
        handleError("tasks_queue malloc fail\n");
    }
    //��ʼ��������
    if(pthread_mutex_init(&(pool->lock), NULL) != 0){
        handleError("lock init fail\n");
    }
    if(pthread_cond_init(&(pool->queue_not_empty), NULL) != 0){
        handleError("queue_not_empty init fail\n");
    }
    if(pthread_cond_init(&(pool->queue_not_full), NULL) != 0){
        handleError("queue_not_full init fail\n");
    }
    // �����߳�
    for(int i = 0; i < thread_num; i++){
        pthread_create(&(pool->threads[i]), NULL, threadpool_op, (void*)pool);
    }
    return pool;
}
void threadpool_add_task(int clnt_sock, threadpool *pool){
    pthread_mutex_lock(&(pool->lock));
    //������������ѭ�����У�����-1
    while(pool->queue_size == pool->queue_max_size - 1){
        pthread_cond_wait(&(pool->queue_not_full), &(pool->lock));
    }
    //���task
    pool->queue_size++;
    pool->tasks_queue[pool->queue_tail] = clnt_sock;
    pool->queue_tail = (pool->queue_tail + 1) % (pool->queue_max_size);
    if(pool->queue_size > 0){
        pthread_cond_broadcast(&(pool->queue_not_empty));    
    }

    pthread_mutex_unlock(&(pool->lock));
}
void *threadpool_op(void *thread_pool){
    threadpool *pool = (threadpool *)thread_pool;
    int task;
    while(1){
        pthread_mutex_lock(&(pool->lock)); //
        //����Ϊ��
        while(pool->queue_size == 0 ){
            //�ȴ����в�Ϊ��
            pthread_cond_wait(&(pool->queue_not_empty), &(pool->lock));
        }
        //ȡ��task
        int clnt_sock = pool->tasks_queue[pool->queue_front];
        pool->queue_size--;
        pool->queue_front = (pool->queue_front + 1) % (pool->queue_max_size);
        //֪ͨ�������µ�task����
        pthread_cond_broadcast(&(pool->queue_not_full));
        //ȡ��������ͷ��̳߳���
        pthread_mutex_unlock(&(pool->lock));
        handle_clnt(clnt_sock);
    }
}
int parse_request(char* request, ssize_t req_len, char* path, ssize_t* path_len){
    char* req = request;
    //�ж��Ƿ�ΪGET������˵������ͷ����
    // printf("%s\n", req);
    if(strlen(req) < 5 || strncmp(req, "GET /", 5) != 0){
        return -1;
    }
    // ��ȡ·��
    size_t begin_index = 3;
    req[begin_index] = '.';
    size_t end_index = 5;
    ssize_t layer = 0;
    
    while (end_index - begin_index < MAX_PATH_LEN){
        if(req[end_index] == ' '){
            if(req[end_index - 1] == '.' && req[end_index - 2] == '.'){
                layer--;
            }
            if(layer < 0){
                return -1; // ��ʾ������ǰĿ¼
            }
            break;
        }
        if(req[end_index] == '/'){
            if(req[end_index - 1] == '.' && req[end_index] == '.'){
                layer--;
            }
            else if(req[end_index - 1] != '.'){
                layer++;
            }
        }
        if(layer < 0){
            return -1;
        }
        end_index++;
    }
    if(end_index - begin_index >= MAX_PATH_LEN){
        return -1;
    }
    memcpy(path, req + begin_index, (end_index - begin_index + 1) * sizeof(char));
    path[end_index - begin_index] = '\0';
    // printf("%s\n", path);
    *path_len = end_index - begin_index;
    // �жϱ�Ҫ���������Ƿ�����
    char *restHead = "HTTP/1.0\r\nHost: 127.0.0.1:8000"; 
    //debug����siege�޷����У��Ǹô�ΪHTTP/1.1
    char *restHead2 = "HTTP/1.1\r\nHost: 127.0.0.1:8000"; 
    if(strncmp(req + end_index + 1, restHead, strlen(restHead)) != 0 &&
    strncmp(req + end_index + 1, restHead2, strlen(restHead)) != 0){
        return -1;
    }
    return 0;
}
void handle_clnt(int clnt_sock){
    // ��ȡ�ͻ��˷����������ݣ�������
    char* req_buf = (char*) malloc( MAX_RECV_LEN * sizeof(char));
    char* req = (char*) malloc( MAX_RECV_LEN * sizeof(char)); 
    char* path = (char*) malloc(MAX_PATH_LEN * sizeof(char));
    char* response = (char*) malloc(MAX_SEND_LEN * sizeof(char)) ;
    if(!req_buf || !req || !path || !response){
        perror("Malloc Error!\n");
        exit(1);
    }
    req[0] = '\0';
    ssize_t req_len = 0;
    int flag = 0;
    // ��ȡ
    while(1){
        req_len = read(clnt_sock, req_buf, MAX_RECV_LEN - 1);  
        if(req_len < 0){ // �����ȡ����
            perror("failed to read client_socket\n");
            exit(1);
        }
        //˵����һ��ѭ��û�ﵽ�˳����������������ݣ����д���
        if(req_len == 0){ // �����ʽ����
            flag = 1;
            break;
        }
        req_buf[req_len] = '\0';
        strcat(req, req_buf);
        //���ĸ��ַ�Ϊ\r\n\r\n, ��˵����ȡ���
        if(strcmp(req + strlen(req) - 4, "\r\n\r\n") == 0){
            break;
        }
    }

    // ���� HTTP ��������ݣ�������Դ·���� Host ͷ
    req_len = strlen(req);
    ssize_t path_len;
    
    int parse_ret = parse_request(req, req_len, path, &path_len);   
  
    //����500
    if(parse_ret == -1 || flag){
        sprintf(response, "HTTP/1.0 %s\r\nContent-Length: %zd\r\n\r\n", HTTP_STATUS_500, (size_t)0);
        size_t response_len = strlen(response);
        if(write(clnt_sock,response,response_len) == -1){
            handleError("Write Error\n");
        }
        close(clnt_sock);
        free(req);
        free(req_buf);
        free(path);
        free(response);
        return;
    }
    // �����ļ�
    // printf("%s\n", path);
    int fd = open(path, O_RDONLY);
    // printf("fd  %d\n",fd);
    if(fd < 0){ //�ļ���ʧ�ܣ�404����
        sprintf(response, "HTTP/1.0 %s\r\nContent-Length: %zd\r\n\r\n", HTTP_STATUS_404, (size_t)0);
        size_t response_len = strlen(response);
        if(write(clnt_sock,response,response_len) == -1){
            handleError("Write Error\n");
            // exit(1);
        }
        close(clnt_sock);
        free(req);
        free(req_buf);
        free(path);
        free(response);
        return;
    }    
    //�ж��ļ��Ƿ���Ŀ¼
    struct stat fs;
    stat(path, &fs);
    if(S_ISDIR(fs.st_mode)){
        sprintf(response, "HTTP/1.0 %s\r\nContent-Length: %zd\r\n\r\n", HTTP_STATUS_500, (size_t)0);
        size_t response_len = strlen(response);
        if(write(clnt_sock,response,response_len) == -1){
            handleError("Write Error\n");
        }
        close(clnt_sock);
        free(req);
        free(req_buf);
        free(path);
        free(response);
        return;
    } 
    //��������   
    sprintf(response,
        "HTTP/1.0 %s\r\nContent-Length: %zd\r\n\r\n",
        HTTP_STATUS_200, (size_t)(fs.st_size));
    size_t response_len = strlen(response);
    if(write(clnt_sock,response,response_len) == -1){
        handleError("Write Error\n");
    }
    //д�ض�ȡ�ļ�����
    while((response_len = read(fd, response, MAX_SEND_LEN)) > 0){
        // printf("res :%s\n", response);
        if(write(clnt_sock, response, response_len) == -1){
           handleError("Write Error\n");
        }
    }
    if(response_len == -1){ //����
        handleError("Read Error\n");
    }
    close(clnt_sock);
    free(req);
    free(req_buf);
    free(path);
    free(response);
}