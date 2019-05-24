#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define FILE_BLOCK_SIZE 512

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

void Sendfile_toserver(char *filename)//客户端传输文件给服务器
{
	char buffer[FILE_BLOCK_SIZE];
	printf("sending %s to the ChatRoom\n", filename);
	FILE *fp = fopen(filename, "r");
	if(fp == NULL)
	{
		printf("Error: File %s not found\n", filename);
		exit(1);
	}
	bzero(buffer, FILE_BLOCK_SIZE);
	int fblock_sz;
	while((fblock_sz = fread(buffer, sizeof(char), FILE_BLOCK_SIZE, fp)) > 0)
	{
		int length = strlen(buffer);
		if(send(sockfd, buffer, length, 0) < 0)
		{
			printf("Error: Failed to send the file %s.\n", filename);
			break;
		} 
		bzero(buffer, FILE_BLOCK_SIZE);
	}	
	printf("OK, File %s was sent successfully!\n", filename);
	fclose(fp);
}

void Recvfile_fromserver(char *filename)//客户端从服务器接收文件
{
	char buffer[FILE_BLOCK_SIZE];
	printf("the recieving filename is: %s\n", filename);
	if(filename == NULL)
	{
		printf("The file isn't existed!\n");
		exit(1);
	}
	FILE *fp = fopen(filename, "w+");
	if(fp == NULL)
	{
		printf("File %s can't be opened!\n", filename);
	}
	else
	{
		bzero(buffer, FILE_BLOCK_SIZE);
		int fblock_sz = 0;
		while(fblock_sz = recv(sockfd, buffer, FILE_BLOCK_SIZE, 0) > 0)
		{
			int length = strlen(buffer);
			printf("the buffer is: %s\n", buffer);
			if(fblock_sz == 0)
			{
				break;
			}
			int write_sz = fwrite(buffer, sizeof(char), length, fp);
			if(write_sz < length)
			{
				error("File write failed!\n");
			}
			bzero(buffer, FILE_BLOCK_SIZE);
			if(fblock_sz == 0 || fblock_sz != FILE_BLOCK_SIZE)
			{
				break;
			}
		}
		if(fblock_sz < 0)
		{
			error("Recieved file error!\n");
		}
		printf("OK recieved from server!\n");
		fclose(fp);
	}
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
		printf("the sending message is: %s\n", msg);
		if (strcmp(buf, "quit") == 0)   //输入quit以退出聊天室
		{
			memset(buf2, 0, sizeof(buf2));
			sprintf(buf2, "  %s 退出聊天室\n", name);
			send(sockfd, buf2, strlen(buf2), 0);
			break;
		}

		if (buf[0] == '-' && buf[1] == 's')//-s: 传输文件，如：-s homework.txt
		{
			char filename[100] = {};
			printf("the client buffer is: %s\n", buf);
			for(int i=3;i<strlen(buf);++i)
				{
					filename[i-3] =buf[i];
				}
			printf("the sending filename is: %s. \n", filename);
			Sendfile_toserver(filename);
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
		
		//接收到 "-s" 提示时，意味着要接收文件了
		char *cmd = "-s ";
		char *fpos = strstr(buf, cmd);
		if(fpos != NULL)
		{
			char filename[100];
			fpos += 3;
			for(int i=0;*fpos != '\0';++fpos, ++i)
			{
				filename[i] = *fpos;
			}
			Recvfile_fromserver(filename);
		}
		
		else
		{
			printf("%s\n", buf);
		}
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
