/*****************************************************************************
** �ļ�: tcp_lib.cpp                                       					**
** ����: ������                                                       				**
** ����: 2010-06-28                                                     					**
** ���ܣ�TCP SOCKET �����⺯��                             	   					**
** �޸ļ�¼
*****************************************************************************/


#include "tcp_lib.h"
//#include "log_lib.h"
#include "log_lib_cplus.h"
#include <sys/epoll.h>
#include <netinet/tcp.h>


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
		logError(log,"���׽���[%d]���������ӳ���.[%d]", iSock, errno );
		return( -1 );
	}
	
	/* ����޶��˿ͻ��˵�IP��ַ	*/
	if ( pcClitHost != NULL && pcClitHost[0] )
	{
		if ( !inet_aton( pcClitHost, &stAddr ) )
		{
			logError(log,"�ͻ��˵�ַ[%s]ת��ʧ��.[%d]", pcClitHost, errno );
			shutdown(*piNewSock, 2);
			close( *piNewSock );
			return( -1 );
		}
		if ( memcmp( &stAddr, &stClit.sin_addr, sizeof(struct in_addr) ) )
		{
			logError(log,"��Ч�Ŀͻ��˵�ַ[%s]", pcClitHost );
			shutdown(*piNewSock, 2);
			close( *piNewSock );
			return( -1 );
		}
	}

	/* ����޶��˿ͻ��˵Ķ˿ں�	*/
	if ( pcClitPort != NULL && pcClitPort[0] )
	{
		hPort = htons( (short)atoi( pcClitPort ) );
		if ( memcmp( &hPort, &stClit.sin_port, sizeof(short) ) )
		{
			logError(log,"��Ч�Ŀͻ��˶˿ں�[%s]", pcClitPort );
			shutdown(*piNewSock, 2);
			close( *piNewSock );
			return( -1 );
		}
	}

	return( 0 );
}


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


	struct linger tmplinger;  // ����linger,ȡ��timewait״̬
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
			logError(log,"ת��������[%s]��IP��ַ����.[%d]", pcHost, errno );
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
				logError(log,"ת��������[%s]���˿ںŴ���.[%d]", pcPort, errno );
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
		logError(log,"�׽���[%d]���󶨴������.[%d]", iSock, errno );
		shutdown(iSock, 2);
		close( iSock );
		return( -1 );
	}

	if ( listen( iSock, 1024) < 0 )
	{
		logError(log,"�׽���[%d]�������������.[%d]", iSock, errno );
		shutdown(iSock, 2);
		close( iSock );
		return( -1 );
	}

	logError(log,"listen succeed iSock=%d", iSock);
	*piSock = iSock;
	return( 0 );
}



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
		logError(log,"�����ͻ����׽���ʧ��.[%d]", errno );
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
   
	/*	����޶��˱�����IP��ַ��˿�,���󶨴���	*/
	if ( ( pcLocHost != NULL && pcLocHost[0] ) || ( pcLocPort != NULL && pcLocPort[0] ) )
	{
		stClit.sin_family = AF_INET;
		if ( pcLocPort != NULL && pcLocPort[0] )
		{
			if ( pcLocPort[0] > '9' || pcLocPort[0] < '0' )
			{
				if ( ( pstServEnt = getservbyname( pcLocPort, "tcp" ) ) ==NULL )
				{
					logError(log,"ת�����ط�����[%s]���˿ںŴ���.[%d]", pcLocPort, errno );
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
				logError(log,"ת������������[%s]��IP��ַ����.[%d]\n", pcLocHost,errno );
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
			logError(log,"�󶨱���������ַ[%s]��˿ں�[%s]����.[%d]\n", pcLocHost, pcLocPort, errno );
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
			logError(log,"ת��Ŀ�������[%s]���˿ںŴ���.[%d]\n", pcTargPort, errno );
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
		logError("ת��Ŀ��������[%s]��IP��ַ����.[%d]\n", pcTargHost, errno );
		shutdown(iSock, 2);
		close( iSock );
		return( -1 );
	}
	memcpy( &stClit.sin_addr, pstHostEnt->h_addr, sizeof(unsigned long) );
*/

	stClit.sin_addr.s_addr = inet_addr(pcTargHost);

	if ( connect( iSock, (struct sockaddr *)&stClit, sizeof(stClit) ) < 0 )
	{
		logError(log,"���ӷ����[%s:%s]ʧ��.[%d]\n", pcTargHost, pcTargPort, errno );
		shutdown(iSock, 2);
		close( iSock );
		return( -1 );
	}

	*piSock = iSock;
	return( 0 ); 
}


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
 
int tcp_send_buf(int iSockfd, const char *psBuf, int iSendLen, int iTimeOut)
{
    
    if (!psBuf || iSendLen <= 0)
    {
       return -1;
    }

    char *ptr = (char *)psBuf;      //ָ�������ݻ���������ʱָ��
    struct timeval timeout;

    fd_set wset;
    int iLenTmp = iSendLen;         //Ҫ����ȫ�����ݵĳ���
    while (iLenTmp > 0)
    {
        timeout.tv_sec  = iTimeOut;
        timeout.tv_usec = 0;

        FD_ZERO(&wset);
        FD_SET(iSockfd, &wset);

        int iRet = select(iSockfd+1, NULL, &wset, NULL, &timeout);
        if (iRet == 0)  //��ʱ
        {
            
            return E_SOCK_TIMEOUT;  
        }
        else if (iRet < 0)   //����
        {
            if (errno == EINTR)
            {
                continue;
            }
            return E_SOCK_ERROR;
        }

        //ÿ��ͨ��sendʵ�ʷ��͵ĳ���
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
int tcp_recv_buf(int iSockfd, char *psRecvBuf, const int iRecvLen, const int iTimeOut)
{
    struct timeval timeout;
    fd_set rset;
    	

    int  iActRecvLen = 0;        //ʵ�ʽ��յ������ݳ���
    int  iLenTmp = iRecvLen;     //Ҫ���յ�ȫ�����ݵĳ���

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
            if (iRet == 0)  //��ʱ
            {
                 
                return E_SOCK_TIMEOUT;  
            }
            else if (iRet < 0)   //����
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
	//��socket����epoll��
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


