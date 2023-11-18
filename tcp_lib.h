/*****************************************************************************
** �ļ�: tcp_lib.h                                       					**
** ����: ������                                                       				**
** ����: 2010-06-28                                                     					**
** ���ܣ�TCP SOCKET �����⺯��ͷ�ļ�                             	   					**
** �޸ļ�¼
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


#define  E_SOCK_TIMEOUT     -1100004	   // ��ʱ			
#define	 E_SOCK_CLOSED      -1100005     // ����������Ҫ�ر�socket	
#define  E_SOCK_ERROR       -1100001  	   // һ�����		

/*****************************************************************************
** ����:  	tcp_accept( )													 
** ���ͣ� 	int																 
** ��Σ� 																	 
**		int  iSock ( ����˼����׽��� )										 
**		int	 *piNewSock ( �½������׽��� )									 
**		char *pcClitHost ( �ͻ��˵�IP��ַ )									 
**		char *pcClitPort ( �ͻ��˵Ķ˿ں� )									 
** ����:  																	 
** ����ֵ�� �ɹ� - 0, ʧ�� - -1               								 
** ���ܣ������µĿͻ��˶����Ӳ������׽���    								 
** ����: liyx
** ����: 2010-06-28
** �޸ļ�¼: 
*****************************************************************************/
int	tcp_accept( int iSock, int *piNewSock, char *pcClitHost, char *pcClitPort );

/*****************************************************************************
** ����:  	tcp_connect( )													 
** ���ͣ� 	int																 
** ��Σ� 																	 
**		char *pcTargHost ( Ŀ���������ƻ�IP��ַ )							 
**		char *pcTargPort ( Ŀ�������������ƻ�˿ں� )						 
**		char *pcLocHost ( �����������ƻ�IP��ַ )							 
**		char *pcLocPort ( ���������������ƻ�˿ں� )						 
**		int	 *piSock ( �������׽��� )										 
** ����:  																	 
** ����ֵ�� �ɹ� - 0, ʧ�� - -1               								 
** ���ܣ�����Tcp�ͻ����׽���    											 
** ����: liyx
** ����: 2010-06-28
** �޸ļ�¼: 
*****************************************************************************/
int	tcp_connect( char *pcTargHost, char *pcTargPort, char *pcLocHost, char *pcLocPort, int *piSock);

/*****************************************************************************
** ����:  	tcp_listen( )														 
** ���ͣ� 	int																 
** ��Σ� 																	 
**		char *pcHost ( �����������ƻ�IP��ַ,�����NULL,���޶�����IP��ַ )	 
**		char *pcPort ( ���������������ƻ�˿ں�,�����0,���ں�ָ����ʱ�˿� ) 
**		int	 *piSock ( �������׽��� )										 
** ����:  																	 
** ����ֵ�� �ɹ� - 0, ʧ�� - -1               								 
** ���ܣ�����Tcp����˼����׽���   
** ����: liyx
** ����: 2010-06-28
** �޸ļ�¼: 
*****************************************************************************/
int	tcp_listen(CLog &log, char *pcHost, char *pcPort, int *piSock );

/********************************************************************************
** �������� : tcp_recv_buf  
** ��    �� : ���ݾ����socket�˿ڷ�����Ϣ
** ������� : int iSockfd        socket�׽ӿھ��
**            char *psRecvBuf    Ҫ���յ����ݻ�����
**            const int iRecvLen Ҫ���յ����ݳ���
**            int iTimeOut       ��ʱʱ��
** ������� : ��
** ����ֵ   : 0 �ɹ� -1 ʧ��
** ����: liyx
** ����: 2010-06-28
** �޸ļ�¼: 
********************************************************************************/
int tcp_recv_buf(int iSockfd, char *psRecvBuf, const int iRecvLen, const int iTimeOut);

/********************************************************************************
** �������� : tcp_send_buf  
** ��    �� : ���ݾ����socket�˿ڷ�����Ϣ
** ������� : int iSockfd        socket�׽ӿھ��
**            const char *psBuf  Ҫ���͵����ݻ�����
**            int iSendLen       Ҫ���͵����ݳ���
**            int iTimeOut       ��ʱʱ��
** ������� : ��
** ����ֵ   : 0 �ɹ� -1 ʧ��
** ����: liyx
** ����: 2010-06-28
** �޸ļ�¼: 
********************************************************************************/
int tcp_send_buf(int iSockfd, const char *psBuf, int iSendLen, int iTimeOut);
int Recv(CLog &log, int fd, char *buf, int size);
int EpollAdd(int sock, int ep_fd);
int DelSockEventFromepoll(int ep_fd, int nSock);
int Send(CLog&log, int nSock, char *pMsg, int nLen);

#endif
