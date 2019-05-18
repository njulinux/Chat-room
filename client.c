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
char password[30]; //存储用户密码

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

int signin(char *name) //客户端登录&注册函数，返回登录结果
{
	int i = 1;
	char k[3] = "unk";
	char get1[3] = {}, get2[3] = {};
	send(sockfd, name, strlen(name), 0);
	recv(sockfd, get1, sizeof(get1), 0);
	if (strcmp(get1, "yes") == 0) //判定名称注册过
	{
		printf("This name have been used.\nAre you new here(please enter yes/no)?");
		scanf("%s", k);
		while (strcmp(k, "yes") !=0 && strcmp(k, "no") != 0)
		{
			printf("please enter [yes/no]:\n");
			scanf("%s", k);
		}
		if (strcmp(k, "yes") == 0) //判断为新用户，重新输入名称，并重新运行函数
		{
			printf("You are new here, please enter another name:");
			scanf("%s", name);
			send(sockfd, "restart", sizeof("restart"), 0); //向服务端发送从头判定的命令
			return signin(name);
		}
		else if (strcmp(k, "no") == 0) //判断为老用户，给予3次输入密码机会
		{
			printf("Please enter your password( you have tried 0/3 times):\n");
			scanf("%s", password);
			send(sockfd, password, strlen(password), 0);
			recv(sockfd, get2, sizeof(get2), 0);
			while(i < 3 && strcmp(get2, "no") == 0)
			{
				printf("password error\n");
				printf("please enter again(you have tried %d/3 times):\n", i);
				scanf("%s", password);
				send(sockfd, password, strlen(password), 0);
				recv(sockfd, get2, sizeof(get2), 0);
				i++;
			}
			if (i < 2 || strcmp(get2, "yes") == 0)
				return 0;
			else
				return -1;
		}
	}
	else if (strcmp(get1, "no") == 0) //判断名称未注册，首次登录视作注册
	{
		printf("You are new user,\nplease enter your password(please don't use restart as password):\n"); //防止restart造成服务端误判定，见上判定为新用户
		scanf("%s", password);
		while (strcmp(password, "restart") == 0)
			{
				printf("Don't use restart as password,and enter another password:");
				scanf("%s", password);
			}
		send(sockfd, password, strlen(password), 0);
		return 0;
	}
	else if(strcmp(get1, "usd") == 0) //判断聊天室中有人使用该名称，重新输入名字，并重新运行函数
	{
		printf("This username have been using,\nplease enter another:\n");
		scanf("%s", name);
		return signin(name);
	}
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
	int check = signin(name);
	if (check == 0) //判断登录&注册成功
		work();
	else //判断登录失败
		printf("login failed,app end\n");
	return 0;
}
