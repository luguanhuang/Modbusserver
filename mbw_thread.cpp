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
#include <vector>
using namespace std;
#include "log_lib_cplus.h"
#include <mysql.h>
#include "modbus.h"
#include "mbw_db.h"

#include "mbw_thread.h"
extern CLog log;

int RecvMsg(int newsocket, int tmout, uint8_t *bbuuff, int req_lenwoCRC)
{	
	fd_set rset; 
	FD_ZERO(&rset);
	FD_SET(newsocket,&rset); 
	int rsw;
	
	struct timeval timeout;
	timeout.tv_sec = (tmout/1000);
	timeout.tv_usec = 0;
	rsw=select(newsocket+1,&rset,NULL,NULL,&timeout);
	
	if (rsw < 0)  //Timeout or error
    {
        return -1;
    }
	else if (0 == rsw)
		return -2;

	int ret = recv(newsocket, bbuuff, req_lenwoCRC, 0);
	logDebug(log, "RecvMsg tmout=%d rsw=%d ret=%d", tmout, rsw,
		ret);
	if(ret <= 0)
    {
        logDebug(log, "server close socket\n");
        return -1;
    }
	
	return ret;
}

int ProcCliPro1Msg(StCliInfo *cliInfo, int &retrycount)
{	
	//modbus write part if pending	
	uint8_t bbuuff[256];
	vector <StWriteDbInfo> vecWDbInfo;
	getmbwriteinfo(cliInfo, vecWDbInfo);
	int retry = 0;
	for (unsigned int i=0; i<vecWDbInfo.size(); i++)
	{
		int iRet = send(cliInfo->newsocket,vecWDbInfo[i].buf,vecWDbInfo[i].len,0);
		if (iRet <= 0)
		{
			logError(log, "send Error\n");
			return -1;
		}
		else
			retry = 0;
		
		int ret = RecvMsg(cliInfo->newsocket, cliInfo->tmout, bbuuff,vecWDbInfo[i].len);
		if(ret < 0) 
		{
			if (ret == -1)
			{
				logError(log, "RecvMsg Error\n");
				return -1;	
			}
			else
			{
				logError(log, "retry is %d \n",retrycount); 
				retry = 1;
			}
		}
		else
			retry = 0;
		int write_success= 0;
		for(int j=0;j<6;j++)
		{  
			if(bbuuff[j] == vecWDbInfo[i].buf[j])
			{
			    write_success =1;
			}
			else
			{
				write_success =0;
				break;
			}
		}
		
		if(write_success == 1)
		{
			updatembinfo(cliInfo, vecWDbInfo[i].id);
			updatechtime(cliInfo, vecWDbInfo[i].ch);
		}	
	}

	vector<StWriteDbInfo>().swap(vecWDbInfo);
	GetDeviceInfo(cliInfo, vecWDbInfo);
	
	for (unsigned int i=0; i<vecWDbInfo.size(); i++)
	{
		int iRet = send(cliInfo->newsocket,vecWDbInfo[i].buf,vecWDbInfo[i].len,0);
		if (iRet <= 0)
		{
			logError(log, "send Error\n");
			
			return -1;
		}
		else
			retry = 0;
		int res_length;		
		int  rsp_header_length= 5;
	    /*for float read response length*/
	    //if(functioncode == 3 | functioncode == 4)
	    if(vecWDbInfo[i].functioncode == 3 || vecWDbInfo[i].functioncode == 4)
	    {
	 		res_length = (vecWDbInfo[i].nb*2)+rsp_header_length;		        
	    }
		
		/*for coil read response length*/
		//if(functioncode == 1 | functioncode == 2)
		if(vecWDbInfo[i].functioncode == 1 || vecWDbInfo[i].functioncode == 2)
		{
			res_length = rsp_header_length + (vecWDbInfo[i].nb / 8) + ((vecWDbInfo[i].nb % 8) ? 1 : 0);
		}
		 
		//printf("select value %d \n",rr);
		logDebug(log, "request is %d \n",vecWDbInfo[i].len);
		for(int j=0;j<vecWDbInfo[i].len;j++)
		{
			printf("<%.2X>", (unsigned char)vecWDbInfo[i].buf[j]);
		}

		printf("\n");
		
		printf("response length should be %d \n",res_length);
		bzero(bbuuff,sizeof(bbuuff));
		int rb = RecvMsg(cliInfo->newsocket, cliInfo->tmout, bbuuff,res_length);
		if(rb < 0) 
		{
			if (-1 == rb)
			{
				logError(log, "Socket Error");
				
				return -1;	
			}
			else
			{					
				logError(log, "retry is %d \n",retrycount); 
				retry = 1;
			}
		}
		else
			retry = 0;
		if(rb == res_length)
		{
			//printf("received success %s \n",bbuuff);
			logDebug(log, "received success11 rb=%d\n", rb);
			for(int i=0;i<rb;i++)
			{
				printf("<%.2X>", (unsigned char)bbuuff[i]);
			}
			printf("\n");
			if(bbuuff[0] == vecWDbInfo[i].slave_id && bbuuff[1]==vecWDbInfo[i].functioncode)
			{   
				
				uint16_t crc_calculated = crc16(bbuuff,res_length - 2);
				uint16_t crc_received = (bbuuff[res_length - 2] << 8) | bbuuff[res_length - 1];
				if(crc_calculated == crc_received)
				{
					if(bbuuff[1] == 3 || bbuuff[1] == 4) 
					{ 
						logDebug(log, "ch=%d nb=%d datatype=%d protocol=%d", vecWDbInfo[i].ch, vecWDbInfo[i].nb,
							vecWDbInfo[i].datatype, cliInfo->protocol);
						if(vecWDbInfo[i].datatype == 2 || vecWDbInfo[i].datatype ==3)
						{
						store_result_holding16(*cliInfo,vecWDbInfo[i].ch,vecWDbInfo[i].nb,vecWDbInfo[i].datatype,(char *)bbuuff,cliInfo->protocol,vecWDbInfo[i].history);
						}
						else if(vecWDbInfo[i].datatype == 4 || vecWDbInfo[i].datatype == 5 || vecWDbInfo[i].datatype == 6 || vecWDbInfo[i].datatype ==7 || vecWDbInfo[i].datatype == 8)
						{
						store_result_reg(*cliInfo,vecWDbInfo[i].ch,vecWDbInfo[i].nb,vecWDbInfo[i].datatype,(char *)bbuuff,cliInfo->protocol,vecWDbInfo[i].history);
					
						}

					}   
					if(bbuuff[1] == 1 || bbuuff[1] == 2)
					{   
						store_result_coil(*cliInfo,vecWDbInfo[i].ch,vecWDbInfo[i].nb,res_length,(char *)bbuuff,cliInfo->protocol,vecWDbInfo[i].history);
					}   
				}
				else
				{
					logError(log, "CRC received error\n");
				}  
			}        
		}//rb==length loop close 
	}
	
	return retry;
}

int ProcCliPro2Msg(StCliInfo *cliInfo, uint16_t &tflag, int &retrycount)
{
	vector <StWriteDbInfo> vecWDbInfo;
	int retry = 0;
	uint8_t bbuuff[256];
	getmbwriteinfo2(cliInfo, vecWDbInfo, tflag);
	for (unsigned int i=0; i<vecWDbInfo.size(); i++)
	{
		int write_success =0;
		int iRet = send(cliInfo->newsocket,vecWDbInfo[i].buf,vecWDbInfo[i].len,0);
		if (iRet <= 0)
		{
			logError(log, "send Error\n");
			return -1;
		}
		else
			retry = 0;
		logDebug(log, "ret=%d", iRet);
		for(int j=0;j<vecWDbInfo[i].len;j++)
		{  			
			write_success =1;
			printf("<%.2X>",j, (unsigned char)vecWDbInfo[i].buf[j]);
		}
		
		printf("\n");
		int rb = RecvMsg(cliInfo->newsocket, cliInfo->tmout, bbuuff,vecWDbInfo[i].req_lenwoCRC);
		if(rb < 0) 
		{
			if (rb == -1)
			{
				logError(log, "Socket Error");
				return -1;	
			}
			else
			{					
				logError(log, "retry is %d \n",retrycount); 
				retry = 1;
			}
		}
		else
			retry = 0;
		for(int j=6;j<10;j++)
		{  
			if(bbuuff[j] == vecWDbInfo[i].buf[j])
			{
				write_success =1;
				printf("bbuuff<%d><%.2X>",j, (unsigned char)bbuuff[j]);
				printf("req_wr<%d><%.2X>",j, (unsigned char)vecWDbInfo[i].buf[j]);
			}
			else{write_success =0;}
		}
		
		//if(rb == req_lenwoCRC )
		if(write_success == 1)
		{
			updatechtime(cliInfo, vecWDbInfo[i].ch);
			updatembinfo(cliInfo, vecWDbInfo[i].id);
		}	
	}
	
	vector<StWriteDbInfo>().swap(vecWDbInfo);
	GetDeviceInfo2(cliInfo, vecWDbInfo, tflag);
	for (unsigned int i=0; i<vecWDbInfo.size(); i++)
	{
		int res_length;
		int rb=0;
		int iRet = send(cliInfo->newsocket,vecWDbInfo[i].buf,vecWDbInfo[i].len,0);
		if (iRet <= 0)
		{
			logError(log, "send Error\n");
			
			return -1;
		}
		else
			retry = 0;
		int  rsp_header_length= 5;
		/*for float read response length*/
		if(vecWDbInfo[i].functioncode == 3 || vecWDbInfo[i].functioncode == 4)
		{
			res_length = ((vecWDbInfo[i].nb*2)+rsp_header_length)+6;
		}
		/*for coil read response length*/
		if(vecWDbInfo[i].functioncode == 1 || vecWDbInfo[i].functioncode == 2)
		{
			res_length = (rsp_header_length + (vecWDbInfo[i].nb / 8) + ((vecWDbInfo[i].nb % 8) ? 1 : 0))+6;
		}

		rb = RecvMsg(cliInfo->newsocket, cliInfo->tmout, bbuuff,res_length);
		if(rb < 0) 
		{
			if (rb == -1)
			{
				printf("Socket Error");
				
				return -1;	
			}
			else
			{					

				logError(log, "retry is %d \n",retrycount); 
				retry = 1;				
			}
		}
		else
			retry = 0;
		
		if(rb == (res_length-2))
		{
			//printf("received success %s \n",bbuuff);
			printf("received success\n");
			for(int j=0;j<rb;j++)
			{
				printf("<%.2X>", (unsigned char)bbuuff[j]);
			}

			printf("\n\n");
			
			if(bbuuff[6] == vecWDbInfo[i].slave_id && bbuuff[7]==vecWDbInfo[i].functioncode)
			{   
			//uint16_t crc_calculated = crc16(bbuuff,res_length - 2);
			//uint16_t crc_received = (bbuuff[res_length - 2] << 8) | bbuuff[res_length - 1];
			//if(crc_calculated == crc_received)
				//  {

				if(bbuuff[7] == 3 || bbuuff[7] == 4) 
					{ 
						logDebug(log, "ch=%d nb=%d datatype=%d protocol=%d", vecWDbInfo[i].ch, vecWDbInfo[i].nb,
							vecWDbInfo[i].datatype, cliInfo->protocol);
						if(vecWDbInfo[i].datatype == 2 || vecWDbInfo[i].datatype ==3)
						{
						store_result_holding16(*cliInfo,vecWDbInfo[i].ch,vecWDbInfo[i].nb,vecWDbInfo[i].datatype,(char *)bbuuff,cliInfo->protocol,vecWDbInfo[i].history);
						}
						else if(vecWDbInfo[i].datatype == 4 || vecWDbInfo[i].datatype == 5 || vecWDbInfo[i].datatype == 6 || vecWDbInfo[i].datatype ==7 || vecWDbInfo[i].datatype == 8)
						{
						store_result_reg(*cliInfo,vecWDbInfo[i].ch,vecWDbInfo[i].nb,vecWDbInfo[i].datatype,(char *)bbuuff,cliInfo->protocol,vecWDbInfo[i].history);
					
						}

					}   
			/*	if(bbuuff[7] == 3 || bbuuff[7] == 4) 
				{ 
					store_result_reg(cliInfo->mysql,cliInfo->user,vecWDbInfo[i].ch,vecWDbInfo[i].nb,vecWDbInfo[i].datatype,(char *)bbuuff,cliInfo->protocol);
				} */   
				if(bbuuff[7] == 1 || bbuuff[7] == 2)
				{   
					store_result_coil(*cliInfo,vecWDbInfo[i].ch,vecWDbInfo[i].nb,res_length,(char *)bbuuff,cliInfo->protocol,vecWDbInfo[i].history);
				}   

				//}
				// else{printf("CRC received error\n");}  
			}        
		}//rb==length loop close     	
	}
	
	return retry;
}

static void *PrcListenMsgThread(void *pParam)
{
	pthread_t thread_id = pthread_self();
	//创建分离线程
	pthread_detach(thread_id);
	StCliInfo *cliInfo = (StCliInfo *)pParam;
	
	uint16_t tflag=0;
	int retrycount=0;
	int ret;
	while (1)
	{
		if(tflag >255)
			tflag = 0;
		/*--------------- program start after protocol select-------*/
		if(cliInfo->protocol == 1)
		{
			ret = ProcCliPro1Msg(cliInfo, retrycount);
	    } 
		else if(cliInfo->protocol == 2)
		{		
			ret = ProcCliPro2Msg(cliInfo, tflag, retrycount);
		}

		if (ret)
		{
			if (ret < 0 || retrycount >= cliInfo->retrynb)
			{
				shutdown(cliInfo->newsocket,SHUT_RDWR);
				close(cliInfo->newsocket); 
				
				UpdateConnectStatus(cliInfo->mysql, cliInfo->id, 0);
				mysql_close(&cliInfo->mysql);
				delete cliInfo;
				return NULL;	
			}
			else
			{
				retrycount++;
			}
		}
		
		sleep(cliInfo->polltime/1000);
	}
	
	return NULL;
}

//connect to the server
int Connect(char *rraddr, int port)
{
	int sock;
	if ((sock = socket(PF_INET, SOCK_STREAM, 0)) == -1)
	{
	    perror("Could not create socket");
	    return -1;
	} 
	
	struct sockaddr_in server;
	
	server.sin_addr.s_addr = inet_addr(rraddr);//Itself is ip
	if (inet_aton(rraddr, &server.sin_addr) == 0)//Fill in the IP address
	{
	    logError(log, "Wrong IP");
	}
	
	server.sin_family = AF_INET;
	server.sin_port = htons((unsigned short)port);	//The port is specified on the command line	

	//connect to the server
	if (connect(sock,
	            (struct sockaddr *) &server,
	            sizeof(server)) == -1)
	{
	    logError(log, "Error: Failed to make connection to %s:%d\n", rraddr, port);
	    return -1;
	}
	
	return sock;
}

static void *ProcConnectThread(void *pParam)
{
	pthread_t thread_id = pthread_self();
	//创建分离线程
	pthread_detach(thread_id);
	StCliInfo *cliInfo = (StCliInfo *)pParam;
	logDebug(log, "ProcConnectThread: func begin protocol=%d", cliInfo->protocol);
	uint16_t tflag=0;
	int retrycount=0;
	int ret = 0;

	int sock = Connect(cliInfo->ip, cliInfo->port);//Connect to the server
	if (-1 == sock)
	{
		logError(log, "ConnectServer Connect error");
		UpdateCloseConnStatus(cliInfo->mysql, cliInfo->id, 0);
		mysql_close(&cliInfo->mysql);
		delete cliInfo;
		return NULL;
	}
	else
	{
		UpdateConnectStatus(cliInfo->mysql, cliInfo->id, 1);
		cliInfo->newsocket = sock;
		logDebug(log, "ConnectServer Connect succeed");	
	}
	
	while (1)
	{
		if(tflag >255)
			tflag = 0;
		/*--------------- program start after protocol select-------*/
		if(cliInfo->protocol == 1)
		{
			ret = ProcCliPro1Msg(cliInfo, retrycount);
	    } 
		else if(cliInfo->protocol == 2)
		{
			ret = ProcCliPro2Msg(cliInfo, tflag, retrycount);		
		}

		if (ret)
		{
			if (ret < 0 || retrycount >= cliInfo->retrynb)
			{
				retrycount = 0;
				shutdown(cliInfo->newsocket,SHUT_RDWR);
				close(cliInfo->newsocket); 
				UpdateCloseConnStatus(cliInfo->mysql, cliInfo->id, 0);
				mysql_close(&cliInfo->mysql);
				delete cliInfo;
				return NULL;
				/*while (1)
				{
					cliInfo->newsocket = Connect(cliInfo->ip, cliInfo->port);//Connect to the server
					if (-1 == cliInfo->newsocket)
					{
						UpdateConnectStatus(cliInfo->mysql, cliInfo->id, 0);
						logError(log, "ConnectServer Connect error sleep 2s and connect again");
						sleep(2);
						//free(cliInfo);
						//return NULL;
					}
					else
					{
						break;	
					}
				}*/
				
				
				//UpdateConnectStatus(cliInfo->mysql, cliInfo->id, 1);	
			
				//free(cliInfo);
				//return NULL;	
			}
			else
			{
				retrycount++;
			}
			
		}
			
		sleep(cliInfo->polltime/1000);
	}
	
	return NULL;
}

int StartThread(StCliInfo *cliInfo, int type)
{		
	int nRet = 0;
	int nStack_size = 1024 * 500;
	pthread_attr_t tattr;

	nRet = pthread_attr_init(&tattr);
	if (0 != nRet)
	{
		logError(log, "StartThread: Call pthread_attr_init error error[%d]=%s", \
			errno, strerror(errno));	
	}
	
	nRet = pthread_attr_setstacksize(&tattr, nStack_size);
	if (0 != nRet)
	{
		logError(log, "StartThread: Call pthread_attr_setstacksize error error[%d]=%s", \
			errno, strerror(errno));		
	}

	pthread_t thread_id;
	
	//创建工作线程
	if (type == 1)
		nRet = pthread_create(&thread_id, &tattr, ProcConnectThread, cliInfo);
	else if (type == 2)
		nRet = pthread_create(&thread_id, &tattr, PrcListenMsgThread, cliInfo);
	if (nRet != 0)
	{
		logError(log, "StartThread: Call pthread_create error error[%d]=%s", \
			errno, strerror(errno));
	}

	return 0;
}

void *MaintainThread(void *pParam)
{
	pthread_t thread_id = pthread_self();
	//创建分离线程
	pthread_detach(thread_id);

	while (1)
	{
		int getuserval; 
		vector <StCliInfo *> vecCliInfo;
		getuserval = getnewuser(vecCliInfo);
		if (getuserval < 0)
		{
			//return -1;
		}

		for (unsigned int i=0; i<vecCliInfo.size(); i++)
		{
			StartThread(vecCliInfo[i], 1);			
		}	
	
		sleep(60);	
	}
	
	return NULL;
}


void *ConnectServer(void *pParam)
{		
	int getuserval; 
	vector <StCliInfo *> vecCliInfo;
	getuserval = getalluser(vecCliInfo);
	if (getuserval < 0)
	{
		//return -1;
	}

	for (unsigned int i=0; i<vecCliInfo.size(); i++)
	{
		StartThread(vecCliInfo[i], 1);					
	}	

	pthread_t thread_id;
	int nRet = pthread_create(&thread_id, NULL, MaintainThread, NULL);
	if (nRet != 0)
	{
		logError(log, "InitThreadPool: Call pthread_create error error[%d]=%s", \
			errno, strerror(errno));
	}
	
	return 0;
}
