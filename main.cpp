#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <termios.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <arpa/inet.h>
//#include <my_global.h>
#include <mysql.h>
#include "log_lib_cplus.h"
#include "mbw_thread.h"
#include "mbw_db.h"

#define PORT 13000
//#define retry 3
int newsocket;
int sockfd;
struct sockaddr_in serverAddr;
struct sockaddr_in newAddr;
char buffer[256];
socklen_t size_addr;
CLog log("log/mb");

int SetSockReuseAddr(int nListen_sock)
{
	
	int nVal = 1;
	//设置socket为可重用端口
	setsockopt(nListen_sock, SOL_SOCKET,SO_REUSEADDR, &nVal, sizeof(nVal));
	return 0;
}

//函数用途:  设置侦听socket,  在程序退出时, 马上关闭TCP连接(防止wait-2 等待时间)
//输入参数:  侦听的 socket id
//输出参数:  无
//返回值	: 设置成功,  返回TRUE， 设置失败， 返回FALSE


int SetsockDonotLinger(int nListen_sock)
{
	
	struct linger lin;
	lin.l_onoff = 1; // 打开linegr开关
	lin.l_linger = 0; // 设置延迟时间为 0 秒, 注意 TCPIP立即关闭，但是有可能出现化身
	setsockopt(nListen_sock, SOL_SOCKET, SO_LINGER, (char *)&lin, sizeof(lin));
	return 0;
}


int main()
{	
	if(log.init(CLOG_DEBUG) < 0)
	{
		fprintf(stderr, "init log faild.\n");
		exit(-1);
	}

    sockfd =socket(AF_INET,SOCK_STREAM,0);
    if(sockfd < 0)
	{
    	logError(log, "error in connection");
    	exit(1);
	}

	SetSockReuseAddr(sockfd);
	SetsockDonotLinger(sockfd);
	
    memset(&serverAddr,'\0',sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    //serverAddr.sin_addr.s_addr = inet_addr("192.168.43.209");
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sockfd,(struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
	    logError(log, "Error in Binding socket");
    	exit(1);
    }
	
	if (listen(sockfd,5) < 0) 
	{
        logError(log, "listen error");
    }

	resetdevicestatus();
	
	logDebug(log, "Listning on port %d \n", PORT);
	pthread_t thread_id;
	//创建工作线程
	int nRet = pthread_create(&thread_id, NULL, ConnectServer, NULL);
	if (nRet != 0)
	{
		logError(log, "InitThreadPool: Call pthread_create error error[%d]=%s", \
			errno, strerror(errno));
	}
	
	sleep(2);
	
	while(1)
	{
    	newsocket = accept(sockfd,(struct sockaddr *)&newAddr,&size_addr);
	    if(newsocket <0)
        {
        	sleep(1);
        	//close(sockfd);        
        	logError(log, "socket accept error\n");
        	continue;
        }
    	else
		{
			logDebug(log, "connection accepted from %s : %d \n",inet_ntoa(newAddr.sin_addr),ntohs(newAddr.sin_port));
		}		

		StCliInfo *cliInfo = new StCliInfo;
		cliInfo->newsocket = newsocket;
        bzero(cliInfo->buffer,sizeof(cliInfo->buffer));
        recv(cliInfo->newsocket, cliInfo->buffer, sizeof(cliInfo->buffer),0);
        logDebug(log, "received buffer is %s", cliInfo->buffer);
		ConnDatabase(cliInfo->mysql);
		
		int getuserval; 
		int cnt = 0;
        getuserval = getuser(cliInfo->buffer, cliInfo, cnt);
        if (getuserval < 0 || cnt <= 0)
        {
            shutdown(newsocket,SHUT_RDWR);
            close(newsocket);
			mysql_close(&cliInfo->mysql);
			continue;
        }

		StartThread(cliInfo, 2);
	}// while accept loop close

	return 0;
}// main loop close

