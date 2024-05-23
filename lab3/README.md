# Lab3 Report
## ʵ������
1. ʹ���̳߳ػ���`10%`
2. ʹ��I/O����epollʵ��`20%`
## �ļ�Ŀ¼�ṹ
```shell
����lab3
��  ����src
��  ��  ����server.c    # �̳߳�ʵ��
��  ��  ����server_epoll.c   # epollʵ��
��  ��  ����unchanged.c   # ԭ����
��  ����README.md     # ʵ�鱨��
```
## �������з���
```shell
# ʹ��gcc����
gcc -g server.c -o server
./server
gcc -g server_epoll.c -o epoll
./epoll
```
## ʵ�����
### �����ͼ���HTTPͷ
��Ҫ����`parse_request`��`handle_clnt`��������
- `handle_clnt(int clnt_sock)` ����
    - ����ѭ����ȡ����ֱ�������õ��ַ����ĺ���λΪ`\r\n\r\n`����Ϊ��ȡ��ɣ�����ڶ�ȡ������`read`��������ֵ`< 0`�������Ϊ�ú���ִ�д�����`= 0`�����ʾ��ȡ��ɵõ�������ͷ����`\r\n\r\n`��������ͷ������
    - ���ŵ���`parse_request` ������������ͷ

- `parse_request` ����
    - ��������ֵ��`-1` ��ʾ `500 Internal Server Error`��`-2` ��ʾ`404 Not Found`��`0`��ʾ����ͷ��ȷ��
    - �õ�������Ϊ`request`�������ж��Ƿ�Ϊ`GET`���󣬲����򷵻�`-1`
    - ����ͨ���жϳ���`../`����`..`�Ĵ�����`/`�Ĵ���֮�����ж��Ƿ�������ǰĿ¼�������򷵻�`-1`��δ������õ��ļ�·����
    - ����ж��Ƿ����`HTTP/1.1\r\nHost: 127.0.0.1:8000`����`HTTP/1.0\r\nHost: 127.0.0.1:8000`���������򷵻�`-1`��

### ��ȡ��������
- ��ȡ����������`handle_clnt(int clnt_sock)` ������ʵ�֡�
- ��`handle_clnt`��������`parse_request` �����󣬵õ��ļ�·����������ͷ�Ϸ�ʱ������`stat`�����ж��Ƿ�ΪĿ¼�����ж��ļ��Ƿ���ڣ��Ӷ�������Ӧ�Ĵ������ļ�·������Ҫ����ѭ����ȡ�ļ����ݲ�д�ؿͻ��ˡ�

### �̳߳�/���̲߳���
- �����̳߳ؽṹ��
    ``` C
    typedef struct{
        pthread_mutex_t lock;
        pthread_cond_t queue_not_full;   // ������
        pthread_cond_t queue_not_empty;  //���в�Ϊ��
        pthread_t *threads;  //����̵߳�tid
        int *tasks_queue;    // �������
        int thread_num; // �߳���
        int queue_front;  //��ͷ
        int queue_tail;   //��β
        int queue_size;   // ��ǰ������
        int queue_max_size; //������󳤶�
    }threadpool;
    ```
- �����̳߳غ���
    ```C
    void *threadpool_op(void *thread_pool);    // ȡ���̳߳���һ������ִ��
    threadpool *threadpool_create(int queue_max_size, int thread_num);  //�����̳߳�
    void threadpool_add_task(int clnt_sock, threadpool *pool); //���̳߳����������
    ```
- ���庯��ʵ��
   `threadpool_create`�������̳߳أ�����ʼ����������������߳�
   `threadpool_add_task`�����ȼ��뻥�����������ж��Ƿ������������ȴ�������������񣬲�֪ͨ�ӷǿգ����⿪��������
   `threadpool_op`�����ȼ��뻥�����������ж��Ƿ�ӿգ��ӿ���ȴ���Ȼ��ȡ�����񣬲�֪ͨ��δ�����⿪�����������ִ������

### I/O����epoll
- ���ñ�Ե���������÷�����
- ����ʵ�ַ�ʽ
    - ���ȴ���`event_list`���󣬲����÷����������ŵ���`epoll_ctl`���Ҫ���õ�`server_socket`��`server_socket`�������ݣ��ı�`event_list`�ӱ�
    - ��`event_list[i].data.fd == server_socket`ʱ����ʾ�������ӣ���ѭ���������󣬲����÷�������������������С�ͬʱ����Ϊ��Ե����`EPOLLIN | EPOLLET`��
    - ��`event_list[i].events == EPOLLIN`ʱ�����ж�������ʱ����`handle_clnt`���������������̳߳���ȣ���handle_clnt������ɾȥ�������ļ�д��ͻ��˵Ĳ��֣�������ļ����������򿪷����ļ���������������event״̬�����򷵻�ֵΪ-1��-2����ʾ��ͬ�������͡�
    - ��`event_list[i].events == EPOLLOUT`ʱ��ִ��д��ͻ��˲����������ϸ����ֵķ���ֵ�ж�д�����ݡ�����ϲ��ֵķ���ֵΪ�ļ�����������ѭ������`sendfile`����д��ͻ��ˣ�ֱ���ļ�д����ȫ�����رտͻ��ˡ�

## ʵ����
### �������
- �������
    ![alt text](png/image.png)
<br>
- Ŀ¼

    ![alt text](png/image-1.png)
<br>
- �ļ�������

    ![alt text](png/image-2.png)
<br>
- ������ǰĿ¼

    ![alt text](png/image-3.png)
<br>
- ����ͷ����GET

    ![alt text](png/image-4.png)

### ���ܲ���
- δ�Ż��汾
    ![alt text](png/image-7.png)
- �̳߳�
    ![alt text](png/image-5.png)
- epoll
    ![alt text](png/image-6.png)

ʹ��`siege -c 100 -r 20 http://127.0.0.1:8000/test.html -v`������в��ԣ����Կ���ʹ���̳߳غ�epoll��`elapsed_time`��`response_time`��`longest_transaction`���������ͣ���`concurrency`��`throughput`��`transaction_rate`���������ߣ�˵��ʹ���̳߳غ�epoll�󲢷��ԣ���Ӧʱ�䣬����Ч�ʵȶ��õ����Ż���
�̳߳غ�epoll��ȣ�����ָ���಻��
**���Ӳ����ļ��Ĵ�С��Լ32K**
�̳߳�
![alt text](png/image-12.png)
epoll
![alt text](png/image-9.png)
**���Ӳ����ļ��Ĵ�С��Լ32K**
epoll
![alt text](png/image-10.png)
�̳߳�
![alt text](png/image-11.png)
���Կ���epoll�Ĳ����Ը�ǿ�������ʱ��϶̣����̳߳ص�`transaction_rate`�ϴ�`troughput`�ϴ����߸������ӡ�