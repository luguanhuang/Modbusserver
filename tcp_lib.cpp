/*****************************************************************************
** 文件: tcp_lib.cpp                                       					**
** 作者: 李勇新                                                       				**
** 日期: 2010-06-28                                                     					**
** 功能：TCP SOCKET 操作库函数                             	   					**
** 修改记录
*****************************************************************************/


#include "tcp_lib.h"
//#include "log_lib.h"
#include "log_lib_cplus.h"
#include <sys/epoll.h>
#include <netinet/tcp.h>


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
int		tcp_accept(CLog &log, int iSock, int *piNewSock, char *pcClitHost, char *pcClitPort )
{
	int		iLen;
	short	hPort;
	struct	sockaddr_in	stClit;
	struct	in_addr		stAddr;


	iLen = sizeof(stClit);
	*piNewSock = accept( iSock, (struct sockaddr *)&stClit, (socklen_t *)&iLen );
	if ( *piNewSock < 0 )
	{
		logError(log,"从套接字[%d]接收新连接出错.[%d]", iSock, errno );
		return( -1 );
	}
	
	/* 如果限定了客户端的IP地址	*/
	if ( pcClitHost != NULL && pcClitHost[0] )
	{
		if ( !inet_aton( pcClitHost, &stAddr ) )
		{
			logError(log,"客户端地址[%s]转换失败.[%d]", pcClitHost, errno );
			shutdown(*piNewSock, 2);
			close( *piNewSock );
			return( -1 );
		}
		if ( memcmp( &stAddr, &stClit.sin_addr, sizeof(struct in_addr) ) )
		{
			logError(log,"无效的客户端地址[%s]", pcClitHost );
			shutdown(*piNewSock, 2);
			close( *piNewSock );
			return( -1 );
		}
	}

	/* 如果限定了客户端的端口号	*/
	if ( pcClitPort != NULL && pcClitPort[0] )
	{
		hPort = htons( (short)atoi( pcClitPort ) );
		if ( memcmp( &hPort, &stClit.sin_port, sizeof(short) ) )
		{
			logError(log,"无效的客户端端口号[%s]", pcClitPort );
			shutdown(*piNewSock, 2);
			close( *piNewSock );
			return( -1 );
		}
	}

	return( 0 );
}


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
int		tcp_listen(CLog &log, char *pcHost, char *pcPort, int *piSock )
{
	int		iSock = -1, iRet = -1;
	struct	sockaddr_in stServ;
	struct	hostent		*pstHostEnt;
	struct	servent		*pstServEnt;
	int nREUSEADDR = 1; 
	int optval;
   int tcp_keepalive_time;
   int tcp_keepalive_interval;
   int tcp_keepalive_probes;
   socklen_t optlen = sizeof(optval);
   
   
	if ( ( iSock = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
	{
		logError(log,"create socket error.[%d]", errno );
		return( -1 );
	}


	struct linger tmplinger;  // 设置linger,取消timewait状态
	int tmplen = sizeof(tmplinger);
	tmplinger.l_onoff  = 1;
	tmplinger.l_linger = 0;
	iRet = setsockopt(iSock, SOL_SOCKET, SO_LINGER, (const char*)&tmplinger, tmplen);
	if(iRet != 0) 
	{
		logError(log, "set SO_LINKER , %s", strerror(errno));
		return( -1 );
	}

	
	
	iRet = setsockopt(iSock, SOL_SOCKET,SO_REUSEADDR,(const char*)&nREUSEADDR, sizeof(int));	
	if(iRet)
	{
		logError(log,"setsockopt() error[%d]", errno );
		return( -1 );
	}
	optval = 1;
	optlen = sizeof(optval);
	tcp_keepalive_time=180;
	tcp_keepalive_interval=60;
	tcp_keepalive_probes=20;
	optlen=sizeof(tcp_keepalive_time);
	iRet = setsockopt(iSock,SOL_TCP,TCP_KEEPIDLE,&tcp_keepalive_time,optlen);
	if(iRet)
	{
		logError(log,"setsockopt() error[%d]", errno );
		return( -1 );
	}
	optlen=sizeof(tcp_keepalive_interval);
	iRet = setsockopt(iSock,SOL_TCP,TCP_KEEPINTVL,&tcp_keepalive_interval,optlen);
	if(iRet)
	{
		logError(log,"setsockopt() error[%d]", errno );
		return( -1 );
	}
	optlen=sizeof(tcp_keepalive_probes);
	setsockopt(iSock,SOL_TCP,TCP_KEEPCNT,&tcp_keepalive_probes,optlen);
	if(iRet)
	{
		logError(log,"setsockopt() error[%d]", errno );
		return( -1 );
	}
	
	
	stServ.sin_family = AF_INET;
	if ( pcHost != NULL && pcHost[0] )
	{
		if ( ( pstHostEnt = gethostbyname( pcHost ) ) == NULL )
		{
			logError(log,"转换主机名[%s]至IP地址错误.[%d]", pcHost, errno );
			shutdown(iSock, 2);
			close( iSock );
			return( -1 );
		}
		memcpy( &stServ.sin_addr.s_addr, pstHostEnt->h_addr, sizeof(unsigned long) );
	}
	else
		stServ.sin_addr.s_addr = htonl(INADDR_ANY);

	if ( pcPort != NULL && pcPort[0] )
	{
		if ( pcPort[0] > '9' || pcPort[0] < '0' )
		{
			if ( ( pstServEnt = getservbyname( pcPort, "tcp" ) ) == NULL )
			{
				logError(log,"转换服务名[%s]至端口号错误.[%d]", pcPort, errno );
				shutdown(iSock, 2);
				close( iSock );
				return( -1 );
			}
			stServ.sin_port = pstServEnt->s_port;
		}
		else	
			stServ.sin_port = htons( (short)atoi( pcPort ) );
	}

	if ( bind( iSock, (struct sockaddr *)&stServ, sizeof(stServ) ) < 0 )
	{
		logError(log,"套接字[%d]做绑定处理出错.[%d]", iSock, errno );
		shutdown(iSock, 2);
		close( iSock );
		return( -1 );
	}

	if ( listen( iSock, 1024) < 0 )
	{
		logError(log,"套接字[%d]做监听处理出错.[%d]", iSock, errno );
		shutdown(iSock, 2);
		close( iSock );
		return( -1 );
	}

	logError(log,"listen succeed iSock=%d", iSock);
	*piSock = iSock;
	return( 0 );
}



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
int	tcp_connect(CLog &log, char *pcTargHost, char *pcTargPort, char *pcLocHost, char *pcLocPort, int *piSock)
{
	int		iSock = -1, iRet = -1;
	
	
	struct	sockaddr_in	stClit;
	struct	servent		*pstServEnt;
	struct	hostent		*pstHostEnt;
	int nREUSEADDR = 1; 
	int optval;
   int tcp_keepalive_time;
   int tcp_keepalive_interval;
   int tcp_keepalive_probes;
   socklen_t optlen = sizeof(optval);

	if ( ( iSock = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
	{
		logError(log,"创建客户端套接字失败.[%d]", errno );
		return( -1 );
	}

	iRet = setsockopt(iSock, SOL_SOCKET,SO_REUSEADDR,(const char*)&nREUSEADDR, sizeof(int));	
	if(iRet)
	{
		logError(log,"setsockopt() error[%d]", errno );
		return( -1 );
	}
	optval = 1;
	optlen = sizeof(optval);
	tcp_keepalive_time=180;
	tcp_keepalive_interval=60;
	tcp_keepalive_probes=20;
	optlen=sizeof(tcp_keepalive_time);
	iRet = setsockopt(iSock,SOL_TCP,TCP_KEEPIDLE,&tcp_keepalive_time,optlen);
	if(iRet)
	{
		logError(log,"setsockopt() error[%d]", errno );
		return( -1 );
	}
	optlen=sizeof(tcp_keepalive_interval);
	iRet = setsockopt(iSock,SOL_TCP,TCP_KEEPINTVL,&tcp_keepalive_interval,optlen);
	if(iRet)
	{
		logError(log,"setsockopt() error[%d]", errno );
		return( -1 );
	}
	optlen=sizeof(tcp_keepalive_probes);
	setsockopt(iSock,SOL_TCP,TCP_KEEPCNT,&tcp_keepalive_probes,optlen);
    if(iRet)
	{
		logError(log,"setsockopt() error[%d]", errno );
		return( -1 );
	}
   
	/*	如果限定了本机的IP地址或端口,做绑定处理	*/
	if ( ( pcLocHost != NULL && pcLocHost[0] ) || ( pcLocPort != NULL && pcLocPort[0] ) )
	{
		stClit.sin_family = AF_INET;
		if ( pcLocPort != NULL && pcLocPort[0] )
		{
			if ( pcLocPort[0] > '9' || pcLocPort[0] < '0' )
			{
				if ( ( pstServEnt = getservbyname( pcLocPort, "tcp" ) ) ==NULL )
				{
					logError(log,"转换本地服务名[%s]至端口号错误.[%d]", pcLocPort, errno );
					shutdown(iSock, 2);
					close( iSock );
					return( -1 );
				}
				stClit.sin_port = pstServEnt->s_port;
			}	
			else
				stClit.sin_port = htons( (short)atoi( pcLocPort ) );
		}
		else
			stClit.sin_port = 0;

		if ( pcLocHost != NULL && pcLocHost[0] )
		{
			if ( ( pstHostEnt = gethostbyname( pcLocHost ) ) == NULL )
			{
				logError(log,"转换本地主机名[%s]至IP地址错误.[%d]\n", pcLocHost,errno );
				shutdown(iSock, 2);
				close( iSock );
				return( -1 );
			}
			memcpy( &stClit.sin_addr,pstHostEnt->h_addr,sizeof(unsigned long) );
		}
		else
			stClit.sin_addr.s_addr = INADDR_ANY;
	
		if ( bind( iSock, (struct sockaddr *)&stClit, sizeof(stClit) ) < 0 )
		{
			logError(log,"绑定本地主机地址[%s]或端口号[%s]错误.[%d]\n", pcLocHost, pcLocPort, errno );
			shutdown(iSock, 2);
			close( iSock );
			return( -1 );
		}
	}


	memset( &stClit, 0, sizeof(stClit) );
	stClit.sin_family = AF_INET;
	if ( pcTargPort[0] > '9' || pcTargPort[0] < '0' )
	{
		if ( ( pstServEnt = getservbyname( pcTargPort, "tcp" ) ) == NULL )
		{
			logError(log,"转换目标服务名[%s]至端口号错误.[%d]\n", pcTargPort, errno );
			shutdown(iSock, 2);
			close( iSock );
			return( -1 );
		}
		stClit.sin_port = pstServEnt->s_port;
	}	
	else
		stClit.sin_port = htons( (short)atoi( pcTargPort ) );

/*
	if ( ( pstHostEnt = gethostbyname( pcTargHost ) ) == NULL )
	{
		logError("转换目标主机名[%s]至IP地址错误.[%d]\n", pcTargHost, errno );
		shutdown(iSock, 2);
		close( iSock );
		return( -1 );
	}
	memcpy( &stClit.sin_addr, pstHostEnt->h_addr, sizeof(unsigned long) );
*/

	stClit.sin_addr.s_addr = inet_addr(pcTargHost);

	if ( connect( iSock, (struct sockaddr *)&stClit, sizeof(stClit) ) < 0 )
	{
		logError(log,"连接服务端[%s:%s]失败.[%d]\n", pcTargHost, pcTargPort, errno );
		shutdown(iSock, 2);
		close( iSock );
		return( -1 );
	}

	*piSock = iSock;
	return( 0 ); 
}


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
 
int tcp_send_buf(int iSockfd, const char *psBuf, int iSendLen, int iTimeOut)
{
    
    if (!psBuf || iSendLen <= 0)
    {
       return -1;
    }

    char *ptr = (char *)psBuf;      //指向发送数据缓冲区的临时指针
    struct timeval timeout;

    fd_set wset;
    int iLenTmp = iSendLen;         //要发送全部数据的长度
    while (iLenTmp > 0)
    {
        timeout.tv_sec  = iTimeOut;
        timeout.tv_usec = 0;

        FD_ZERO(&wset);
        FD_SET(iSockfd, &wset);

        int iRet = select(iSockfd+1, NULL, &wset, NULL, &timeout);
        if (iRet == 0)  //超时
        {
            
            return E_SOCK_TIMEOUT;  
        }
        else if (iRet < 0)   //出错
        {
            if (errno == EINTR)
            {
                continue;
            }
            return E_SOCK_ERROR;
        }

        //每次通过send实际发送的长度
        int iLen = send(iSockfd, ptr, iLenTmp, 0);
        if ( iLen <  iLenTmp )
        {
            if (iLen == -1)
            {
                if ((errno != EINTR) || (errno != EWOULDBLOCK) || (errno != EMSGSIZE))
                {
                    return E_SOCK_CLOSED;
                }
                else if (errno == EWOULDBLOCK)
                {
                    return E_SOCK_ERROR;
                }

                continue;
            }

            iLenTmp -= iLen;
            ptr += iLen;
        }
        else
        {
            iLenTmp -= iLen;
        }
    }

    //printf("sendbuf success %d\n", iSendLen);
    return 0;
}


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
int tcp_recv_buf(int iSockfd, char *psRecvBuf, const int iRecvLen, const int iTimeOut)
{
    struct timeval timeout;
    fd_set rset;
    	

    int  iActRecvLen = 0;        //实际接收到的数据长度
    int  iLenTmp = iRecvLen;     //要接收的全部数据的长度

   char* pBuf = (char *)psRecvBuf;

    while (iLenTmp > 0)
    {
        if ( iTimeOut )
        {
            timeout.tv_sec  = iTimeOut;
            timeout.tv_usec = 0;

            FD_ZERO(&rset);
            FD_SET(iSockfd, &rset);

            int iRet = select(iSockfd+1, &rset, NULL, NULL, &timeout);
            if (iRet == 0)  //超时
            {
                 
                return E_SOCK_TIMEOUT;  
            }
            else if (iRet < 0)   //出错
            {
                if (errno == EINTR)
                {
                    continue;
                }
                return E_SOCK_ERROR;
            }
        }

        int iLen = recv(iSockfd, pBuf, iLenTmp, 0);
        if (iLen == -1)
        {
            if ((errno != EINTR) || (errno != EWOULDBLOCK))
            {
                return E_SOCK_CLOSED;
            }
            else if (errno == EWOULDBLOCK)
            {
                return E_SOCK_ERROR;
            }
            else
            {
                continue;
            }
        }
        else if (iLen == 0)
        {
            return E_SOCK_CLOSED;
        }
        else
        {
            iActRecvLen += iLen;

            if (iLen < iLenTmp)
            {
                pBuf += iLen;
            }

            iLenTmp -= iLen;
        }
    }

    return iActRecvLen;
}

#define MAX_TRY_CNT 10

int Recv(CLog &log, int fd, char *buf, int size)
{
	int recvsum = 0;
	int ret;
	int trycnt = 0;
	
	while (recvsum < size)
	{
		if (trycnt++ > MAX_TRY_CNT)
		{
			return -1;
		}
		
		ret = recv(fd, buf + recvsum, size - recvsum, 0);
		
		if (ret <= 0)
		{
			if (errno == EAGAIN)
			{
				logDebug(log, "Recv: recv EAGAIN");
				//DEBUG("Recv: recv EAGAIN\n");
				usleep(100000);	
				continue;
			}
			
			logError(log, "Recv: recv failure ret=%d err[%d]=%s\n", ret, errno, strerror(errno));
			return -1;
		}

		recvsum += ret;
	}
	
	return 0;
}

#define SEND_LEN 10240
int Send(CLog&log, int nSock, char *pMsg, int nLen)
{
	int nTotal_len = nLen;
	int nSnd_len = 0;
	int nRet = 0;
	int nRemain_len = nTotal_len - nSnd_len;
	int cnt = 0;
	
	while (nRemain_len > 0)
	{
		if (cnt++ > MAX_TRY_CNT)
		{
			break;	
		}
		
		nRemain_len = nRemain_len > SEND_LEN ? SEND_LEN : nRemain_len;
		nRet = send(nSock, pMsg + nSnd_len, nRemain_len, 0);
		logDebug(log, "ret=%d", nRet);
		if (-1 == nRet)
		{
			if (EAGAIN == errno)
			{
				logDebug(log, "wait usleep(10000)");
				usleep(10000);
				continue;	
			}
			else
			{
				logError(log, "Send: Call send error [return value]=%d [%d]=%s nSock=%d\n", 
						nRet, errno, strerror(errno), nSock);
				return nRet;			
			}
		}
		
		nSnd_len += nRet;
		nRemain_len = nTotal_len - nSnd_len;
		logDebug(log, "ret=%d nSnd_len=%d nRemain_len=%d", 
			nRet, nSnd_len, nRemain_len);
	}

	//printf("Send: send data [data Len]=%d [socket id]=%d\n", nLen, nSock);
	return nSnd_len;
}

int EpollAdd(int sock, int ep_fd)
{
	uint32_t event_type = EPOLLIN | EPOLLET;
	int nRet = 0;
	struct epoll_event ev = {0, {0}};
	//把socket放入epoll中
	ev.events = event_type;
	ev.data.fd = sock;
	nRet = epoll_ctl(ep_fd, EPOLL_CTL_ADD, sock, &ev);	
	if (nRet < 0)
	{
		return -1;
	}
	return 0;
}

int DelSockEventFromepoll(int ep_fd, int nSock)
{
	struct epoll_event ev = {0, {0}};
	epoll_ctl(ep_fd, EPOLL_CTL_DEL, nSock, &ev);	
}


