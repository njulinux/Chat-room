#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

int sockfd;
char *IP = "127.0.0.1"; //IP地址
short PORT = 8000;  //端口号
typedef struct sockaddr SA;
char name[30]; //存储昵称

void init() //客户端初始化函数
{
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = inet_addr(IP);
	if (connect(sockfd, (SA *) & addr, sizeof(addr)) == -1) //建立与服务器的TCP连接
	{
		perror("connect error!");
		exit(-1);
	}
	printf("客户端初始化成功\n");
}

void signin(char *name) //客户端登录函数，判断是否重复
{
	char get[3] = {};
	send(sockfd, name, strlen(name), 0);
	recv(sockfd, get, sizeof(get), 0);
	while (strcmp(get, "yes") == 0)
	{
		printf("This name has been used,\n");
		printf("please enter another name:");
		scanf("%s", name);
		send(sockfd, name, strlen(name), 0);
		recv(sockfd, get, sizeof(get), 0);
	}
	printf("This name hasn't been used\n");
}

void enter(char *buf) //完善空格输入问题
{
	char buf1[100] = {};
	int i = 0;
	fgets(buf1, 100, stdin);
	for (i = 0; buf1[i] != 0; i++)
		buf[i] = buf1[i];
	buf[i - 1] = '\0';
}

void work() //客户端工作函数
{
	pthread_t id;
	void *client_thread(void *);
	pthread_create(&id, 0, client_thread, 0); //创建线程
	char buf2[100] = { };
	sprintf(buf2, "%s 进入聊天室\n", name); //sprintf 把格式化的数据写入某个字符串中，把进入聊天室的信息写入buf2
	send(sockfd, buf2, strlen(buf2), 0); //进入聊天室信息发送至服务器端
	while (1) 
	{
		char buf[100] = { }; //存放聊天内容
		enter(buf);
		char msg[131] = { };
		sprintf(msg, "%s:%s", name, buf);
		send(sockfd, msg, strlen(msg), 0); //聊天信息发送至服务器端
		if (strcmp(buf, "quit") == 0)   //输入quit以退出聊天室
		{
			memset(buf2, 0, sizeof(buf2));
			sprintf(buf2, "  %s 退出聊天室\n", name);
			send(sockfd, buf2, strlen(buf2), 0);
			break;
		}
	}
	close(sockfd);
}

 void *client_thread(void *p)  //客户端线程函数，利用死循环不停的从服务器端接收消息，并打印到屏幕上
{
	while (1) 
	{
		char buf[100] = { };
		if (recv(sockfd, buf, sizeof(buf), 0) <= 0) 
		{
			printf("error!");
			exit(-1);
		}
		printf("%s\n", buf);
	}
}

 int main()
{
	init();
	printf("please input your name: ");
	scanf("%s", name);
	signin(name);
	work();
	return 0;
}
