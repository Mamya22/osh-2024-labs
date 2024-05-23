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
#include <sys/epoll.h>
#include <sys/sendfile.h>
#include <errno.h>
// #include <pthread.h>

#define BIND_IP_ADDR "127.0.0.1"
#define BIND_PORT 8000
#define MAX_RECV_LEN 1048576
#define MAX_SEND_LEN 1048576
#define MAX_PATH_LEN 1024
#define MAX_HOST_LEN 1024
#define MAX_CONN 20
// #define THREAD_MAX_NUM 100
// #define QUEUE_SIZE 1000
#define FD_SIZE 1000
#define MAX_EVENTS 200
#define MAX  0x3fffffff

#define HTTP_STATUS_200 "200 OK"
#define HTTP_STATUS_500 "500 Internal Server Error"
#define HTTP_STATUS_404 "404 Not Found"
// int fd_ret[MAX];
//记录相关信息
typedef struct Relate{
    int fd;
    int ret;
    size_t size;
}Relate;

void handleError(char *error){
    perror(error);
    exit(EXIT_FAILURE);
}
int parse_request(char* request, ssize_t req_len, char* path, ssize_t* path_len);
int handle_clnt(Relate *relate, int clnt_sock);
int handle_epoll(int server_socket);
void handle_write(int fd, int clnt_sock, size_t  size);
int main(){
    // 创建套接字，参数说明：
    //   AF_INET: 使用 IPv4
    //   SOCK_STREAM: 面向连接的数据传输方式
    //   IPPROTO_TCP: 使用 TCP 协议
    int serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	int opt = 1;
	setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

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
    int flag = fcntl(serv_sock,F_GETFL, 0);
    flag |= O_NONBLOCK;
    fcntl(serv_sock, F_SETFL, flag);
    //设置非阻塞
    handle_epoll(serv_sock);

    // 实际上这里的代码不可到达，可以在 while 循环中收到 SIGINT 信号时主动 break
    // 关闭套接字
    close(serv_sock);
    return 0;
}
size_t num =0;
int handle_epoll(int server_socket){
    //创建epoll
    int epoll_fd;
    if((epoll_fd = epoll_create(FD_SIZE)) == -1){
        handleError("failed to create epoll\n");
    }
    struct epoll_event event_list[MAX_EVENTS];
    struct epoll_event event;
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addr_size = sizeof(clnt_addr);
    event.data.fd = server_socket;
    event.events = EPOLLIN | EPOLLET;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event) == -1){
        handleError("epoll_ctl fail\n");
    }
    // size_t size_ret[MAX_EVENTS] = {0};
    int evnum = 0;
    while (1){
        if((evnum = epoll_wait(epoll_fd, event_list, MAX_EVENTS, -1)) == -1){
            handleError("epoll_wait error\n");
        }
        else{
           
            printf("num   %ld\n", num); num++;
        }
        for(int i = 0; i < evnum; i++){
            // if ((event_list[i].events & EPOLLERR) ||
			// 	(event_list[i].events & EPOLLHUP)) { // 连接出错
			// 	fprintf(stderr, "epoll error\n");
			// 	close(event_list[i].data.fd);
			// 	continue;
			// }
            if(event_list[i].data.fd == server_socket) { //有连接
                    printf("event_list[i].data.fd i:%d %d\n", i,event_list[i].data.fd);
                while(1){
                    int clnt_sock = accept(server_socket, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
                    //设置非阻塞
                    if (clnt_sock == -1) {
						if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
							break;
						} else
							handleError("Accept failed");
					}
                    int flag = fcntl(clnt_sock, F_GETFL, 0);
                    flag |= O_NONBLOCK;
                    fcntl(clnt_sock, F_SETFL, flag);
                    Relate *relate = (Relate *)malloc(sizeof(Relate));
                    relate->fd = clnt_sock;
                    event.data.ptr = (void *)relate;
                    event.events = EPOLLIN | EPOLLET;
                    // event.data.fd = clnt_sock;
                    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, clnt_sock, &event) == -1){
                        handleError("epoll_ctl fail\n");
                    }
                }
            }
            else if(event_list[i].events == EPOLLIN){
                // size_t size;
                Relate *relate = (Relate *)event_list[i].data.ptr;
                int clnt_ret = handle_clnt(relate, relate->fd);

                //用数组记录对应记录返回值
                // size_ret[i] = size;

                if(clnt_ret != -1 && clnt_ret != -2){
                    event.events   = EPOLLOUT | EPOLLET;
                    event.data.ptr = (void *)relate;
                    if(epoll_ctl(epoll_fd, EPOLL_CTL_MOD, relate->fd, &event) == -1){
                        handleError("epoll_ctl fail\n");
                    }
                }
            }
            else if(event_list[i].events == EPOLLOUT){
                printf("event_list[i].events i:%d %d\n", i,event_list[i].events);
                Relate *relate = (Relate *)event_list[i].data.ptr;
                handle_write(relate->ret, relate->fd,  relate->size);
                free(relate);
            }
            else{
                Relate *relate = (Relate *)event_list[i].data.ptr;
                close(relate->fd);
            }
        }
    }
    
}
void handle_write(int fd, int clnt_sock, size_t size){
    char* response = (char*) malloc(MAX_SEND_LEN * sizeof(char)) ;
    if(!response){
        handleError("Malloc Error!\n");
    }
    if(fd == -1){

        sprintf(response, "HTTP/1.0 %s\r\nContent-Length: %zd\r\n\r\n", HTTP_STATUS_500, (size_t)0);
        size_t response_len = strlen(response);
        if(write(clnt_sock,response,response_len) == -1){
            handleError("Write Error\n");
        }
    }
    if(fd == -2){
        sprintf(response, "HTTP/1.0 %s\r\nContent-Length: %zd\r\n\r\n", HTTP_STATUS_404, (size_t)0);
        size_t response_len = strlen(response);
        if(write(clnt_sock,response,response_len) == -1){
            perror("Write Error\n");
            exit(1);
        } 
    }
    //正常处理 
    else{
        sprintf(response,
            "HTTP/1.0 %s\r\nContent-Length: %zd\r\n\r\n",
            HTTP_STATUS_200, (size_t)(size));
        size_t response_len = strlen(response);
        if(write(clnt_sock,response,response_len) == -1){
            perror("Write Error\n");
            exit(1);
        }
        //写回读取文件内容
        // while((response_len = read(fd, response, MAX_SEND_LEN)) > 0){
        //     // printf("res :%s\n", response);
        //     if(write(clnt_sock, response, response_len) == -1){
        //         perror("Write Error\n");
        //         exit(1);
        //     }
        // }
        size_t temp_size = size;
        while (temp_size > 0){
           int ret = sendfile(clnt_sock, fd, NULL, size);
            if(ret < 0){
                handleError("error sendfile\n");
            }
            temp_size = temp_size - ret;
        }
        
        // if(response_len == -1){ //错误
        //     perror("Read Error\n");
        //     exit(1);
        // }        
        close(fd);
    }  
    close(clnt_sock);
    free(response);

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
    char *restHead2 = "HTTP/1.1\r\nHost: 127.0.0.1:8000"; 
    if(strncmp(req + end_index + 1, restHead, strlen(restHead)) != 0 &&
    strncmp(req + end_index + 1, restHead2, strlen(restHead)) != 0){
        return -1;
    }
    return 0;
}
int handle_clnt(Relate *relate, int clnt_sock){
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
        free(req);
        free(req_buf);
        // free(path);
        free(response);  
    if(parse_ret == -1){
        free(path);
        relate->ret = -1;
        return -1;
    }
    // 处理文件
    int fd = open(path, O_RDONLY);
    // printf("fd%d\n",fd);
    if(fd < 0){ //文件打开失败，4004错误
        free(path);
        relate->ret = -2;
        return -2;
    }    
    //判断文件是否是目录
    struct stat fs;
    stat(path, &fs);
    if(S_ISDIR(fs.st_mode)){
        free(path);
        relate->ret = -1;
        return -1;
    } 

    // printf("%ld\n", fs.st_size); 
    relate->size = fs.st_size;
    free(path);
    relate->ret = fd;
    return fd;
}
