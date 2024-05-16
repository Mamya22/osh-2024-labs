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
#define MAX_CONN 20
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
    pthread_t *threads;  //存放线程的tid
    int *tasks_queue;
    int thread_num; // 线程数
    int queue_front;
    int queue_tail;
    int queue_size; // 任务数
    int queue_max_size; //最大任务数
    int shutdown;  //是否关闭
}threadpool;


int parse_request(char* request, ssize_t req_len, char* path, ssize_t* path_len);
void handle_clnt(int clnt_sock);
threadpool *threadpool_create(int queue_max_size, int thread_num);
void threadpool_add_task(int clnt_sock, threadpool *pool);
void *threadpool_op(void *thread_pool);
int main(){
    // 创建套接字，参数说明：
    //   AF_INET: 使用 IPv4
    //   SOCK_STREAM: 面向连接的数据传输方式
    //   IPPROTO_TCP: 使用 TCP 协议
    int serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // 将套接字和指定的 IP、端口绑定
    //   用 0 填充 serv_addr（它是一个 sockaddr_in 结构体）
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    //   设置 IPv4
    //   设置 IP 地址
    //   设置端口
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(BIND_IP_ADDR);
    serv_addr.sin_port = htons(BIND_PORT);
    //   绑定
    bind(serv_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));

    // 使得 serv_sock 套接字进入监听状态，开始等待客户端发起请求
    listen(serv_sock, MAX_CONN);

    // 接收客户端请求，获得一个可以与客户端通信的新的生成的套接字 clnt_sock
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size = sizeof(clnt_addr);
    threadpool *pool = threadpool_create(QUEUE_SIZE,THREAD_MAX_NUM);
    while(1){
        // 当没有客户端连接时，accept() 会阻塞程序执行，直到有客户端连接进来
        int clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
        // 处理客户端的请求
        // handle_clnt(clnt_sock);
        if(clnt_sock == -1){
            handleError("accept error\n");
        }
        threadpool_add_task(clnt_sock, pool);
    }

    // 实际上这里的代码不可到达，可以在 while 循环中收到 SIGINT 信号时主动 break
    // 关闭套接字
    close(serv_sock);
    return 0;
}
threadpool *threadpool_create(int queue_max_size, int thread_num){
    threadpool *pool = (threadpool *)malloc(sizeof(threadpool));
    if(!pool){
        handleError("threadpool malloc fail\n");
    }
    pool->queue_front = 0;
    pool->queue_size  = 0;
    pool->thread_num = thread_num;
    pool->queue_tail = 0;
    pool->shutdown = 0;
    pool->queue_max_size = queue_max_size;
    //为线程开辟相应的空间
    pool->threads = (pthread_t *)malloc(thread_num * sizeof(pthread_t));
    if(!pool->threads){
        handleError("threads malloc fail\n");
    }
    pool->tasks_queue = (int *)malloc(sizeof(int) * queue_max_size);
    if(!pool->tasks_queue){
        handleError("tasks_queue malloc fail\n");
    }
    //初始化互斥锁
    if(pthread_mutex_init(&(pool->lock), NULL) != 0){
        handleError("lock init fail\n");
    }
    if(pthread_cond_init(&(pool->queue_not_empty), NULL) != 0){
        handleError("queue_not_empty init fail\n");
    }
    if(pthread_cond_init(&(pool->queue_not_full), NULL) != 0){
        handleError("queue_not_full init fail\n");
    }
    // 启动线程
    for(int i = 0; i < thread_num; i++){
        pthread_create(&(pool->threads[i]), NULL, threadpool_op, (void*)pool);
    }
    return pool;
}


void threadpool_add_task(int clnt_sock, threadpool *pool){
    pthread_mutex_lock(&(pool->lock));
    //队满则阻塞
    while(pool->queue_size == pool->queue_max_size){
        pthread_cond_wait(&(pool->queue_not_full), &(pool->lock));
    }
    pool->queue_size++;
    pool->tasks_queue[pool->queue_tail] = clnt_sock;
    pool->queue_tail = (pool->queue_tail + 1) % (pool->queue_max_size);
    pthread_cond_broadcast(&(pool->queue_not_empty));
    pthread_mutex_unlock(&(pool->lock));
}
void *threadpool_op(void *thread_pool){
    threadpool *pool = (threadpool *)thread_pool;
    int task;
    while(1){
        pthread_mutex_lock(&(pool->lock)); //
        //队列为空
        while(pool->queue_size == 0 && (!pool->shutdown)){
            //等待队列不为空
            pthread_cond_wait(&(pool->queue_not_empty), &(pool->lock));
        }
        int clnt_sock = pool->tasks_queue[pool->queue_front];
        pool->queue_size--;
        pool->queue_front = (pool->queue_front + 1) % (pool->queue_max_size);
        pthread_cond_broadcast(&(pool->queue_not_full));
        //取出任务后释放线程池锁
        pthread_mutex_unlock(&(pool->lock));
        handle_clnt(clnt_sock);
    }
}
int parse_request(char* request, ssize_t req_len, char* path, ssize_t* path_len){
    char* req = request;
    //判断是否为GET，以下说明请求头不对
    if(strlen(req) < 5 || strncmp(req, "GET /", 5) != 0){
        return -1;
    }
    // 获取路径
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
                return -1; // 表示跳出当前目录
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
    // 判断必要请求内容是否完整
    char *restHead = "HTTP/1.0\r\nHost: 127.0.0.1:8000"; 
    if(strncmp(req + end_index + 1, restHead, strlen(restHead)) != 0){
        return -1;
    }
    return 0;
}
void handle_clnt(int clnt_sock){
    // 读取客户端发送来的数据，并解析
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
    // 读取
    while(1){
        req_len = read(clnt_sock, req_buf, MAX_RECV_LEN - 1);  
        if(req_len < 0){ // 处理读取错误
            perror("failed to read client_socket\n");
            exit(1);
        }
        //说明上一次循环没达到退出条件，但读完数据，即有错误
        if(req_len == 0){ // 处理格式错误
            perror("Error request tail\n");
            exit(1);
        }
        req_buf[req_len] = '\0';
        strcat(req, req_buf);
        //后四个字符为\r\n\r\n, 则说明读取完成
        if(strcmp(req + strlen(req) - 4, "\r\n\r\n") == 0){
            break;
        }
    }
    // 根据 HTTP 请求的内容，解析资源路径和 Host 头
    req_len = strlen(req);
    ssize_t path_len;
    
    int parse_ret = parse_request(req, req_len, path, &path_len);   
  
    //处理500
    if(parse_ret == -1){
        sprintf(response, "HTTP/1.0 %s\r\nContent-Length: %zd\r\n\r\n", HTTP_STATUS_500, (size_t)0);
        size_t response_len = strlen(response);
        if(write(clnt_sock,response,response_len) == -1){
            perror("Write Error\n");
            exit(1);
        }
        close(clnt_sock);
        free(req);
        free(req_buf);
        free(path);
        free(response);
        return;
    }
    // 处理文件
    int fd = open(path, O_RDONLY);
    printf("fd%d\n",fd);
    if(fd < 0){ //文件打开失败，4004错误
        sprintf(response, "HTTP/1.0 %s\r\nContent-Length: %zd\r\n\r\n", HTTP_STATUS_404, (size_t)0);
        size_t response_len = strlen(response);
        if(write(clnt_sock,response,response_len) == -1){
            perror("Write Error\n");
            exit(1);
        }
        close(clnt_sock);
        free(req);
        free(req_buf);
        free(path);
        free(response);
        return;
    }    
    //判断文件是否是目录
    struct stat fs;
    stat(path, &fs);
    if(S_ISDIR(fs.st_mode)){
        sprintf(response, "HTTP/1.0 %s\r\nContent-Length: %zd\r\n\r\n", HTTP_STATUS_500, (size_t)0);
        size_t response_len = strlen(response);
        if(write(clnt_sock,response,response_len) == -1){
            perror("Write Error\n");
            exit(1);
        }
        close(clnt_sock);
        free(req);
        free(req_buf);
        free(path);
        free(response);
        return;
    } 
    //正常处理   
    sprintf(response,
        "HTTP/1.0 %s\r\nContent-Length: %zd\r\n\r\n",
        HTTP_STATUS_200, (size_t)(fs.st_size));
    size_t response_len = strlen(response);
    if(write(clnt_sock,response,response_len) == -1){
        perror("Write Error\n");
        exit(1);
    }
    //写回读取文件内容
    while((response_len = read(fd, response, MAX_SEND_LEN)) > 0){
        printf("res :%s\n", response);
        if(write(clnt_sock, response, response_len) == -1){
            perror("Write Error\n");
            exit(1);
        }
    }
    if(response_len == -1){ //错误
        perror("Read Error\n");
        exit(1);
    }
    close(clnt_sock);
    free(req);
    free(req_buf);
    free(path);
    free(response);
}
