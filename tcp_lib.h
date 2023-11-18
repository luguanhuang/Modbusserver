/*****************************************************************************
** 文件: tcp_lib.h                                       					**
** 作者: 李勇新                                                       				**
** 日期: 2010-06-28                                                     					**
** 功能：TCP SOCKET 操作库函数头文件                             	   					**
** 修改记录
*****************************************************************************/

#ifndef  __TCP_LIB_H__
#define  __TCP_LIB_H__

#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <ctype.h>
//#include <popt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>

#include "log_lib_cplus.h"


#define  E_SOCK_TIMEOUT     -1100004	   // 超时			
#define	 E_SOCK_CLOSED      -1100005     // 致命错误，需要关闭socket	
#define  E_SOCK_ERROR       -1100001  	   // 一般错误		

/*****************************************************************************
** 函数:  	tcp_accept( )													 
** 类型： 	int																 
** 入参： 																	 
**		int  iSock ( 服务端监听套接字 )										 
**		int	 *piNewSock ( 新建立的套接字 )									 
**		char *pcClitHost ( 客户端的IP地址 )									 
**		char *pcClitPort ( 客户端的端口号 )									 
** 出参:  																	 
** 返回值： 成功 - 0, 失败 - -1               								 
** 功能：接收新的客户端端连接并返回套接字    								 
** 作者: liyx
** 日期: 2010-06-28
** 修改记录: 
*****************************************************************************/
int	tcp_accept( int iSock, int *piNewSock, char *pcClitHost, char *pcClitPort );

/*****************************************************************************
** 函数:  	tcp_connect( )													 
** 类型： 	int																 
** 入参： 																	 
**		char *pcTargHost ( 目标主机名称或IP地址 )							 
**		char *pcTargPort ( 目标主机服务名称或端口号 )						 
**		char *pcLocHost ( 本地主机名称或IP地址 )							 
**		char *pcLocPort ( 本地主机服务名称或端口号 )						 
**		int	 *piSock ( 建立的套接字 )										 
** 出参:  																	 
** 返回值： 成功 - 0, 失败 - -1               								 
** 功能：建立Tcp客户端套接字    											 
** 作者: liyx
** 日期: 2010-06-28
** 修改记录: 
*****************************************************************************/
int	tcp_connect( char *pcTargHost, char *pcTargPort, char *pcLocHost, char *pcLocPort, int *piSock);

/*****************************************************************************
** 函数:  	tcp_listen( )														 
** 类型： 	int																 
** 入参： 																	 
**		char *pcHost ( 本地主机名称或IP地址,如果是NULL,不限定本地IP地址 )	 
**		char *pcPort ( 本地主机服务名称或端口号,如果是0,由内核指定临时端口 ) 
**		int	 *piSock ( 建立的套接字 )										 
** 出参:  																	 
** 返回值： 成功 - 0, 失败 - -1               								 
** 功能：建立Tcp服务端监听套接字   
** 作者: liyx
** 日期: 2010-06-28
** 修改记录: 
*****************************************************************************/
int	tcp_listen(CLog &log, char *pcHost, char *pcPort, int *piSock );

/********************************************************************************
** 函数名称 : tcp_recv_buf  
** 描    述 : 根据具体的socket端口发送信息
** 输入参数 : int iSockfd        socket套接口句柄
**            char *psRecvBuf    要接收的数据缓冲区
**            const int iRecvLen 要接收的数据长度
**            int iTimeOut       超时时间
** 输出参数 : 无
** 返回值   : 0 成功 -1 失败
** 作者: liyx
** 日期: 2010-06-28
** 修改记录: 
********************************************************************************/
int tcp_recv_buf(int iSockfd, char *psRecvBuf, const int iRecvLen, const int iTimeOut);

/********************************************************************************
** 函数名称 : tcp_send_buf  
** 描    述 : 根据具体的socket端口发送信息
** 输入参数 : int iSockfd        socket套接口句柄
**            const char *psBuf  要发送的数据缓冲区
**            int iSendLen       要发送的数据长度
**            int iTimeOut       超时时间
** 输出参数 : 无
** 返回值   : 0 成功 -1 失败
** 作者: liyx
** 日期: 2010-06-28
** 修改记录: 
********************************************************************************/
int tcp_send_buf(int iSockfd, const char *psBuf, int iSendLen, int iTimeOut);
int Recv(CLog &log, int fd, char *buf, int size);
int EpollAdd(int sock, int ep_fd);
int DelSockEventFromepoll(int ep_fd, int nSock);
int Send(CLog&log, int nSock, char *pMsg, int nLen);

#endif
