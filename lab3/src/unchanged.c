// server.c
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BIND_IP_ADDR "127.0.0.1"
#define BIND_PORT 8000
#define MAX_RECV_LEN 1048576
#define MAX_SEND_LEN 1048576
#define MAX_PATH_LEN 1024
#define MAX_HOST_LEN 1024
#define MAX_CONN 20

#define HTTP_STATUS_200 "200 OK"

void parse_request(char* request, ssize_t req_len, char* path, ssize_t* path_len)
{
    char* req = request;

    // һ���ֲڵĽ��������������� BUG��
    // ��ȡ��һ���ո� (s1) �͵ڶ����ո� (s2) ֮������ݣ�Ϊ PATH
    ssize_t s1 = 0;
    while(s1 < req_len && req[s1] != ' ') s1++;
    ssize_t s2 = s1 + 1;
    while(s2 < req_len && req[s2] != ' ') s2++;

    memcpy(path, req + s1 + 1, (s2 - s1 - 1) * sizeof(char));
    path[s2 - s1 - 1] = '\0';
    *path_len = (s2 - s1 - 1);
}

void handle_clnt(int clnt_sock)
{
    // һ���ֲڵĶ�ȡ������������ BUG��
    // ��ȡ�ͻ��˷����������ݣ�������
    char* req_buf = (char*) malloc(MAX_RECV_LEN * sizeof(char));
    // �� clnt_sock ��Ϊһ���ļ�����������ȡ��� MAX_RECV_LEN ���ַ�
    // ��һ�ζ�ȡ������֤�Ѿ������������ȡ����
    ssize_t req_len = read(clnt_sock, req_buf, MAX_RECV_LEN);

    // ���� HTTP ��������ݣ�������Դ·���� Host ͷ
    char* path = (char*) malloc(MAX_PATH_LEN * sizeof(char));
    ssize_t path_len;
    parse_request(req_buf, req_len, path, &path_len);

    // ����Ҫ���ص�����
    // ����û��ȥ��ȡ�ļ����ݣ������Է���������Դ·����Ϊʾ����������Զ���� 200
    // ע�⣬��Ӧͷ������Ҫ��һ�����໻�У�\r\n\r\n����Ȼ�������Ӧ����
    char* response = (char*) malloc(MAX_SEND_LEN * sizeof(char)) ;
    sprintf(response,
        "HTTP/1.0 %s\r\nContent-Length: %zd\r\n\r\n%s",
        HTTP_STATUS_200, path_len, path);
    size_t response_len = strlen(response);

    // ͨ�� clnt_sock ��ͻ��˷�����Ϣ
    // �� clnt_sock ��Ϊ�ļ�������д����
    write(clnt_sock, response, response_len);

    // �رտͻ����׽���
    close(clnt_sock);

    // �ͷ��ڴ�
    free(req_buf);
    free(path);
    free(response);
}

int main(){
    // �����׽��֣�����˵����
    //   AF_INET: ʹ�� IPv4
    //   SOCK_STREAM: �������ӵ����ݴ��䷽ʽ
    //   IPPROTO_TCP: ʹ�� TCP Э��
    int serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

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

    while (1) // һֱѭ��
    {
        // ��û�пͻ�������ʱ��accept() ����������ִ�У�ֱ���пͻ������ӽ���
        int clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
        // ����ͻ��˵�����
        handle_clnt(clnt_sock);
    }

    // ʵ��������Ĵ��벻�ɵ�������� while ѭ�����յ� SIGINT �ź�ʱ���� break
    // �ر��׽���
    close(serv_sock);
    return 0;
}