#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define FILE_BLOCK_SIZE 512
#define DEFARG(fd) ((#fd[0]) ? (fd+0): -1)

int sockfd;
int fds[100];  //存储accept得到的套接字
int size = 100;
char *IP = "127.0.0.1"; //IP地址
short PORT = 8000;     //端口号
typedef struct sockaddr SA;
char *namelist[100]; //名称列表，存储当前聊天室中的名称
 
void init()  //服务器端初始化函数
{
	sockfd = socket(AF_INET, SOCK_STREAM, 0); //创建socket套接字
	if (sockfd == -1) 
	{
		perror("Creat socket error!");
		exit(-1);
	}
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));  //memset函数清零
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = inet_addr(IP);
	if (bind(sockfd, (SA *) & addr, sizeof(addr)) == -1) //套接字与网络地址绑定
	{
		perror("bind error");
		exit(-1);
	}
	if (listen(sockfd, 100) == -1)  //监听
	{
		perror("listen error");
		exit(-1);
	}
	for (int i = 0; i < 100; i++) //初始化聊天室昵称列表
		namelist[i] = NULL;
}

void SendMsg2All(char *msg, int fd)  //消息发送函数，将msg发给所有客户端
{
	int i;
	for (i = 0; i < size; i++) 
	{
		if (fds[i] != 0 && fds[i] != fd) 
		{
			printf("send to %d\n", fds[i]);
			send(fds[i], msg, strlen(msg), 0);
		}
	}
}

int checkused(char *name) //名称检查函数，检查聊天室名称列表中是否有输入的名称
{
	int address, n = 0;
	for (address = 0; address < 100; address++)
	{
		if (namelist[address] != NULL)
			if (strcmp(name, namelist[address]) == 0)
			{
				n = 1;
				break;
			}
	}
	return n;
}

void useradd(char *name) //名称添加函数，将输入的名称添加入聊天室名称列表
{
	int address;
	for (address = 0; address < 100; address++)
		if (namelist[address] == NULL)
			break;
	namelist[address] = (char*)malloc(sizeof(char) * (strlen(name) + 1));
	strcpy(namelist[address], name);
}

void userdelete(char *name) //名称删除函数，将输入名称从聊天室名称列表中删除
{
	int address;
	for (address = 0; address < 100; address++)
		if (namelist[address] != NULL)
			if (strcmp(name, namelist[address]) == 0)
				break;
	if (address < 100)
	{
		free(namelist[address]);
		namelist[address] = NULL;
	}
}

void checkrecv(int fd, char *taget) //接受数据函数，在recv基础上，判断是否断连
{
	char buf[30] = {};
	if (recv(fd, buf, sizeof(buf), 0) <= 0)
	{
		int i;
		for (i = 0; i < size; i++)
		{
			if (fd == fds[i])
			{
				fds[i] = 0;
				break;
			}
		}
		printf("exit: fd = %d 退出\n", fd);
		pthread_exit( &i);
	}
	strcpy(taget, buf);
}

void Recvfile_fromclient(char *filename, int fd)
{
	if(filename == NULL)
	{
		error("No file recieved!\n");
	}
	printf("filename is: %s\n", filename);
	char buffer[FILE_BLOCK_SIZE] = {};
	FILE *fp = fopen(filename, "w");
	if(fp == NULL)
	{
		printf("File %s can't be opened!\n", filename);
	}
	else
	{
		bzero(buffer, FILE_BLOCK_SIZE);
		int fblock_sz = 0;
		while(fblock_sz = recv(fd, buffer, sizeof(buffer), 0) > 0)
		{
			int length = strlen(buffer);
			printf("the buffer length is: %d\n", length);
			if(fblock_sz == 0)
			{
				break;
			}
			int write_sz = fwrite(buffer, sizeof(char), length, fp);
			printf("the size if: %d\n", write_sz);
			if(write_sz < fblock_sz)
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
		printf("OK recieved from client!\n");
		fclose(fp);
	}
}

void Sendfile_toclients(char *filename, int fd)
{
	for (int i = 0; i < size; i++) 
	{
		if (fds[i] != 0 && fds[i] != fd) 
		{
			printf("send file: %s to %d\n", filename, fds[i]);
			char buffer[FILE_BLOCK_SIZE];
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
				if(send(fds[i], buffer, length, 0) < 0)
				{
					printf("Error: Failed to send the file %s\n", filename);
					break;
				} 
				bzero(buffer, FILE_BLOCK_SIZE);
			}	
			printf("OK, File %s was sent successfully!\n", filename);
			fclose(fp);
		}
	}

}

void server_thread(void *p, char *name) //服务器端线程函数，利用多线程转发消息
{
	int fd = *(int *) p;
	printf("pthread = %d 进入\n", fd);
	while (1) 
	{
		char buf[100] = { };
		if (recv(fd, buf, sizeof(buf), 0) <= 0)  //判断连接中止或发生错误
		{
			int i;
			for (i = 0; i < size; i++) 
			{
				if (fd == fds[i]) 
				{
					fds[i] = 0;				
					break;
				}
			}
			userdelete(name);
			printf("exit: fd = %d 退出\n", fd);
			pthread_exit( &i);
		}
		char *cmd = "-s ";//判断有client发送文件
		char *fpos = strstr(buf, cmd);
		printf("the sever buffer is: %s\n",  buf);
		if(fpos != NULL)
		{
			char filename[100];
			fpos += 3;
			for(int i=0;*fpos != '\0';++fpos, ++i)
			{
				filename[i] = *fpos;
			}
			Recvfile_fromclient(filename, fd);
			SendMsg2All(buf, DEFARG(fd));
			Sendfile_toclients(filename, fd);
		}
		else
		{
			SendMsg2All(buf, DEFARG());
		}
	} 
}  

void *checkin(void *p) //服务端判断登入函数
{
	int fd = *(int *) p;
	int n = -1;
	char name[30] = {};
	checkrecv(fd, name);
	n = checkused(name); //判断名称是否存在于聊天室名称列表中
	while(n != 0)
	{
		send(fd, "yes", strlen("yes"), 0);
		checkrecv(fd, name);
		n = checkused(name);
	}
	send(fd, "no", strlen("no"), 0);
	useradd(name);
	server_thread(p, name);
}

void service() //服务器工作函数
{
	printf("聊天室服务器端启动 \n");
	while (1) 
	{
		struct sockaddr_in cliaddr;
		socklen_t len = sizeof(cliaddr);
		int connfd = accept(sockfd, (SA *) & cliaddr, &len); //返回新的套接字描述符，代表与客户端的连接
		if (connfd == -1) 
		{
			printf("Something wrong with connection...\n");
			continue;
		}
		int i = 0;
		for (i = 0; i < size; i++) 
		{
			if (fds[i] == 0) 
			{
				fds[i] = connfd;  //fds数组存储connfd
				printf("connfd = %d\n", connfd);
				pthread_t tid;  //声明线程ID
				pthread_create(&tid, 0, checkin,&connfd); //创建线程
				break;
			}
			if (size == i) 
			{
				char *str = "Sorry, the chat room is full!";
				send(connfd, str, strlen(str), 0);
				close(connfd);
			}
			
		}
       	}
}  

int main()
{
	init();
	service();
	return 0;
}


