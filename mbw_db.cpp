#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <mysql.h>
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
#include <string.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "log_lib_cplus.h"
#include "modbus.h"
#include "mbw_db.h"
#include <vector>

using namespace std;
extern CLog log;
map <string, string> g_mapdata; 



int ConnDatabase(MYSQL &mysql)
{
	mysql_library_end();
	mysql_init(&mysql);
	mysql_options(&mysql, MYSQL_READ_DEFAULT_GROUP, "");
	
	if (mysql_real_connect(&mysql, "localhost", "root", "root1", 
    	"cloud", 0, NULL, 0) == NULL) 
    {
	    logError(log, "mysql_real_connet() failure %s\n", mysql_error(&mysql));
	    mysql_close(&mysql);
    	return -1;
    }

	if(mysql_autocommit(&mysql, 1))  // auto commit
	{
		logError(log, "mysql_autocommit() failure: [%s].", mysql_error(&mysql));
		mysql_close(&mysql);
		return -1;
	}
	
	return 0;
}

int IsStringNotEmpty(char *pBuf)
{	
	int nlen = strlen(pBuf);
	if (0 == nlen)
	{
		return 0;
	}

	return 1;
}

int ReconnDatabase(MYSQL &conns)
{
	int nConn_count = 0;
	while (1)
	{
		if (ConnDatabase(conns))					//连接数据库
		{
			nConn_count++;
			if (4 == nConn_count)
			{
				logError(log, "ReconnDatabase: reconnect the DB connection exceed the max connect num then"
					" give up and exit the func [max connect num]=%d", 4);
				break;
			}

			logError(log, "ReconnDatabase: connect DB error sleep 5s and then connect again");
			sleep(5);
			continue;	
		}		
		else
		{			
			break;	
		}
	}
	return 0;
}

int HandleDBError(MYSQL &pConn)
{

	//获取数据库的错误码
	int nMysql_errno = mysql_errno(&pConn);
	//如果是数据库断连
	if (CR_SERVER_LOST == nMysql_errno || CR_SERVER_GONE_ERROR == nMysql_errno)
	{
		logError(log, "HandleDBError: DB connection is disconnect mysql_error[%d]=%s", \
									mysql_errno(&pConn), mysql_error(&pConn));
		ReconnDatabase(pConn);			//重连数据库
		
		return -1;	
	}
	else
	{
		//如果是数据库的其他错误
		logError(log, "HandleDBError: DB have other error mysql_error[%d]=%s", \
		mysql_errno(&pConn), mysql_error(&pConn));		
		return -1;					
	}
}

int GetDataFromTable(MYSQL &conns, char *pSQL, MYSQL_RES *&ppMysql_res)
{
	if (mysql_query(&conns, pSQL))
	{
		logError(log, "GetDataFromTable: Call mysql_query error%s", "");
		ppMysql_res = NULL;
		HandleDBError(conns);
		return -1;
	}

	ppMysql_res = mysql_store_result(&conns);
	if (NULL == ppMysql_res)
	{
		HandleDBError(conns);
		return -1;
	}
	
	return 0;
}

int UpdateConnectStatus(MYSQL &mysql, int id,int status)
{
	char query[300];
	sprintf(query,"update auth set connected='%d',threaded='1' where id=%d",
		status, id); 
	
	logDebug(log, "slq=%s\n", query);
	if (mysql_query(&mysql, query)) 
	{
		logError(log, "%s\n", mysql_error(&mysql));
		return -1;
	}
	
	return 0;
}      

int UpdateCloseConnStatus(MYSQL &mysql, int id,int status)
{
	char query[300];
	sprintf(query,"update auth set connected='%d',threaded='0' where id=%d",
		status, id); 
	
	logDebug(log, "slq=%s\n", query);
	if (mysql_query(&mysql, query)) 
	{
		logError(log, "%s\n", mysql_error(&mysql));
		return -1;
	}
	
	return 0;
}    


int getChsInfo(MYSQL &mysql,char *devname, vector <StChInfo> &vecChInfo)
{
	char query[300];
	sprintf(query,"SELECT ch,chdesc from %s where Alarmchannel='1' and active='1'",
		devname); 
	

	if (mysql_query(&mysql, query)) 
	{
		logError(log, "%s\n", mysql_error(&mysql));
		// mysql_close(con);
		return -1;
	}


	MYSQL_RES *result = mysql_store_result(&mysql);
	if (result == NULL) 
	{
		logError(log, "%s\n", mysql_error(&mysql));
		return -1;
	}
	
	mysql_num_fields(result);
	MYSQL_ROW row;


	StChInfo chInfo;
	while((row = mysql_fetch_row(result))) 
	{               
		memset(&chInfo, 0, sizeof(chInfo));
		chInfo.chid = atoi(row[0]);
		snprintf(chInfo.chdesc, sizeof(chInfo.chdesc),
			"%s", row[1]);
		logDebug(log, "getChsInfo id is %d\n",chInfo.chid);
		vecChInfo.push_back(chInfo);
	} 
	
	mysql_free_result(result);
	return 0;
}

int resetdevicestatus()
{
	MYSQL mysql;
	ConnDatabase(mysql);
	char query[300];
	sprintf(query,"update auth set threaded='0',connected='0'"); 
	
	logDebug(log, "resetdevicestatus=%s\n", query);
	if (mysql_query(&mysql, query)) 
	{
		logError(log, "%s\n", mysql_error(&mysql));
		// mysql_close(con);
		return -1;
	}
	
	return 0;
}     


int getuser(char buff[], StCliInfo *clientInfo, int &cnt)
{
	char query[300];
	sprintf(query,"SELECT devname,ptype,retry,timeout, polltime, id from auth where macid='%s' and active='1' and socktype='1'",
		buff); 
	
	logDebug(log, "slq=%s\n", query);
	if (mysql_query(&clientInfo->mysql, query)) 
	{
		logError(log, "%s\n", mysql_error(&clientInfo->mysql));
		// mysql_close(con);
		return -1;
	}
	MYSQL_RES *result = mysql_store_result(&clientInfo->mysql);
	if (result == NULL) 
	{
		logError(log, "%s\n", mysql_error(&clientInfo->mysql));
		return -1;
	}
	
	mysql_num_fields(result);
	MYSQL_ROW row;

	while((row = mysql_fetch_row(result))) 
	{                
		strcpy(clientInfo->user,row[0]);
		clientInfo->protocol = atoi(row[1]);
		clientInfo->retrynb = atoi(row[2]);
		clientInfo->tmout = atoi(row[3]);
		clientInfo->polltime = atoi(row[4]);
		clientInfo->id = atoi(row[5]);
		logDebug(log, "getuser user is %s\n",clientInfo->user);
		logDebug(log,"getuser protocol is %d\n",clientInfo->protocol);
		logDebug(log,"getuser retrycount is %d\n",clientInfo->retrynb);
		logDebug(log,"getuser timeout is %d\n",clientInfo->tmout);
		logDebug(log,"getuser polltime is %d\n",clientInfo->polltime);
		logDebug(log,"getuser id is %d\n",clientInfo->id);
		getChsInfo(clientInfo->mysql,clientInfo->user,clientInfo->vecChInfo);
		cnt++;
	} 
	
	mysql_free_result(result);

	UpdateConnectStatus(clientInfo->mysql, clientInfo->id, 1);
	
	return 0;
}      

int updatembinfo(StCliInfo *cliInfo, int id)
{
	char wqu[256];
	sprintf(wqu,"UPDATE %s_write set mbwrite=('1') where id=%d",cliInfo->user,id); 
 
	if (mysql_query(&cliInfo->mysql, wqu)) 
    {
		logError(log, "%s\n", mysql_error(&cliInfo->mysql));
    }
    else
    	logDebug(log, "database update success\n");
	return 0;
}
std::string getCurrentTimeStr(){
  time_t t = time(NULL);
  char ch[64] = {0};
  char result[100] = {0};
  strftime(ch, sizeof(ch) - 1, "%Y%m%d--%H%M%S", localtime(&t));
  sprintf(result, "%s", ch);
  return std::string(result);
}

int updatechtime(StCliInfo *cliInfo, int chid)
{
	char wqu[256];
	string curtime = getCurrentTimeStr();
	//"update % set writelogin='$user',writedatetime='$time' where ch=".$channel
	sprintf(wqu,"UPDATE %s set writeackdatetime='%s' where ch=%d",cliInfo->user,curtime.c_str(),chid); 
 
	if (mysql_query(&cliInfo->mysql, wqu)) 
    {
		logError(log, "%s\n", mysql_error(&cliInfo->mysql));
    }
    else
    	logDebug(log, "updatechtime database update success wqu=%s\n", wqu);
	return 0;
}


int getmbwriteinfo(StCliInfo *cliInfo, vector <StWriteDbInfo> &vecWDbInfo)
{
	//modbus write part if pending
	char wq[256];		
	sprintf(wq,"SELECT id,channel,dtype,slaveid,funcode,reg,value from %s_write WHERE mbwrite=('0')",cliInfo->user); 
	logDebug(log, "sql=%s", wq);
	if (mysql_query(&cliInfo->mysql, wq)) 
	{
		logError(log, "%s", mysql_error(&cliInfo->mysql));
		mysql_close(&cliInfo->mysql);
		return -1;
	}
	
	MYSQL_RES *resultw = mysql_store_result(&cliInfo->mysql);
	if (resultw == NULL) 
	{
		logError(log, "%s\n", mysql_error(&cliInfo->mysql));
	}

	MYSQL_ROW roww;
	StWriteDbInfo dbInfo;
	
	while((roww = mysql_fetch_row(resultw))) 
	{ 
		int id = atoi(roww[0]);
		int dtype = atoi(roww[2]);
		int sl_id = atoi(roww[3]);
		int funcode = atoi(roww[4]);
		int reg = (atoi(roww[5])-1);
		dbInfo.id = id;
		int size_float=4;
		dbInfo.buf[0] = sl_id;
		dbInfo.buf[1] = funcode;
		dbInfo.buf[2] = reg >> 8;
		dbInfo.buf[3] = reg & 0x00ff;
		
		if(dtype == 1 && funcode == 5)
		{                      
		    dbInfo.len =6;
			if(atoi(roww[6]) > 0)
			{
				dbInfo.buf[4] = 0XFF;
				dbInfo.buf[5] = 0x00;
			}
		  	else
			{
		        dbInfo.buf[4] = 0X00;
		        dbInfo.buf[5] = 0x00;
		    }

		}

		//float write values
		//if((dtype == 4 | dtype == 5 | dtype == 6 | dtype == 7 | dtype == 8) && funcode == 16)
		if((dtype == 4 || dtype == 5 || dtype == 6 || dtype == 7 || dtype == 8) && funcode == 16)
		{   
		    //uint16_t req_w_16;
		    dbInfo.buf[4] = 0X00;
		    dbInfo.buf[5] = 0X02;
		    dbInfo.buf[6] = size_float;                 
		    dbInfo.len = 6 + 1 + size_float;//1 is extra byte to get data byte receive
		    uint32_t ik;
		    uint16_t dest[2];
		    float fir = atof(roww[6]);
		    memcpy(&ik,&fir, sizeof(uint32_t));
			if(dtype == 4) 
			{
				ik = htonl(ik); 
				dest[0] = ik;
				dest[1] = ik >> 16;
			}
			
			if(dtype == 5) 
			{
				ik = htonl(ik);
				dest[0] = ik >> 16;
				dest[1] = ik ;
			}
			
		    if(dtype == 6) 
		    {
				ik = bswap_32(htonl(ik));
				dest[0] = ik >> 16;
				dest[1] = ik;
		    }
			
		   	if(dtype == 7) 
		    {
		        ik = htonl(ik);
		        dest[0] = (uint16_t)bswap_16(ik >> 16);
		        dest[1] = (uint16_t)bswap_16(ik & 0xFFFF);
		    }
			
		    if(dtype == 8) 
			{
			     //ik = htonl(ik);
			     dest[0] = ik;
			     dest[1] = ik >> 16;
			}        
		   
		    logDebug(log, "dest[0] %.4X",dest[0]);
		    logDebug(log, "dest[1] %.4X",dest[1]);
		    dbInfo.buf[7] = dest[0] ;
		    dbInfo.buf[8] = dest[0]>> 8;
		    dbInfo.buf[9] = dest[1] ;
		    dbInfo.buf[10] = dest[1]>> 8;

		}

		if((dtype == 2 || dtype == 3) && funcode == 06)
		{         
			if(dtype == 3)
			{             
			uint16_t fir = atoi(roww[6]);
			 dbInfo.len =6;
		   // memcpy(&ik,&fir, sizeof(uint32_t));
		    dbInfo.buf[4] = fir >> 8;
			dbInfo.buf[5] = fir ;
			}
			if (dtype == 2)
			{
			int16_t fir = atoi(roww[6]);
			 dbInfo.len =6;
		   // memcpy(&ik,&fir, sizeof(uint32_t));
		    dbInfo.buf[4] = fir >> 8;
			dbInfo.buf[5] = fir ;
			}
	
		}
		uint16_t crcw = crc16(dbInfo.buf,dbInfo.len);
		dbInfo.buf[dbInfo.len++] = crcw >> 8;
		dbInfo.buf[dbInfo.len++] = crcw & 0X00FF;
		vecWDbInfo.push_back(dbInfo);
	}

	mysql_free_result(resultw);
	return 0;
}

int getmbwriteinfo2(StCliInfo *cliInfo, vector <StWriteDbInfo> &vecWDbInfo, uint16_t &tflag)
{
	//modbus write part if pending
	char wq[256]; 


	StWriteDbInfo dbInfo;
	
	sprintf(wq,"SELECT id,channel,dtype,slaveid,funcode,reg,value from %s_write WHERE mbwrite=('0')",cliInfo->user); 
	if (mysql_query(&cliInfo->mysql, wq)) 
	{
		logError(log, "%s\n", mysql_error(&cliInfo->mysql));
		
		mysql_close(&cliInfo->mysql);
		shutdown(cliInfo->newsocket,SHUT_RDWR);
		close(cliInfo->newsocket);
		return -1;
	}

	logDebug(log, "getmbwriteinfo2:wq=%s", wq);
	MYSQL_RES *resultw = mysql_store_result(&cliInfo->mysql);
	if (resultw == NULL) 
	{
		logError(log, "%s\n", mysql_error(&cliInfo->mysql));
	}

	MYSQL_ROW roww;

	while((roww = mysql_fetch_row(resultw))) 
	{ 
		dbInfo.len = 6;
		dbInfo.id = atoi(roww[0]);
		int dtype = atoi(roww[2]);
		int sl_id = atoi(roww[3]);
		int funcode = atoi(roww[4]);
		int reg = (atoi(roww[5])-1);
		
		int size_float=4;
		dbInfo.buf[0] = tflag >> 8;
		dbInfo.buf[1] = tflag;
		dbInfo.buf[2] =0X00;
		dbInfo.buf[3] =0X00;
		dbInfo.buf[dbInfo.len++] = sl_id;
		dbInfo.buf[dbInfo.len++] = funcode;
		dbInfo.buf[dbInfo.len++] = reg >> 8;
		dbInfo.buf[dbInfo.len++] = reg & 0x00ff;

		if(dtype == 1 && funcode == 5)
		{                      
		dbInfo.req_lenwoCRC =6+1+5;
		//res_length_offset =6;
		if(atoi(roww[6]) > 0)
		{
		dbInfo.buf[dbInfo.len++] = 0XFF;
		dbInfo.buf[dbInfo.len++] = 0x00;
		}
		else{
		dbInfo.buf[dbInfo.len++] = 0X00;
		dbInfo.buf[dbInfo.len++] = 0x00;
		}

		}
		//float write values
		//if((dtype == 4 | dtype == 5 | dtype == 6 | dtype == 7 | dtype == 8) && funcode == 16)
		if((dtype == 4 || dtype == 5 || dtype == 6 || dtype == 7 || dtype == 8) && funcode == 16)
		{   
			//uint16_t req_w_16;
			//res_length_offset =6;
			dbInfo.buf[dbInfo.len++] = 0X00;
			dbInfo.buf[dbInfo.len++] = 0X02;
			dbInfo.buf[dbInfo.len++] = size_float;                 
			dbInfo.req_lenwoCRC = 6+1 + 1 + size_float;//1 is extra byte to get data byte receive
			uint32_t ik;
			uint16_t dest[2];
			float fir = atof(roww[6]);
			memcpy(&ik,&fir, sizeof(uint32_t));
			if(dtype == 4) 
			{
				ik = htonl(ik); 
				dest[0] = ik;
				dest[1] = ik >> 16;
			}
			if(dtype == 5) 
			{
				ik = htonl(ik);
				dest[0] = ik >> 16;
				dest[1] = ik ;
			}
			if(dtype == 6) 
			{
				ik = bswap_32(htonl(ik));
				dest[0] = ik >> 16;
				dest[1] = ik ;
			}
			if(dtype == 7) 
			{
				ik = htonl(ik);
				dest[0] = (uint16_t)bswap_16(ik >> 16);
				dest[1] = (uint16_t)bswap_16(ik & 0xFFFF);
			}
			
			if(dtype == 8) 
			{
				//ik = htonl(ik);
				dest[0] = ik;
				dest[1] = ik >> 16;
			}        

			logDebug(log, "dest[0] %.4X",dest[0]);
			logDebug(log, "dest[1] %.4X",dest[1]);
			dbInfo.buf[dbInfo.len++] = dest[0] ;
			dbInfo.buf[dbInfo.len++] = dest[0]>> 8;
			dbInfo.buf[dbInfo.len++] = dest[1] ;
			dbInfo.buf[dbInfo.len++] = dest[1]>> 8;
		}
		if((dtype == 2 || dtype == 3) && funcode == 06)
		{         
			if(dtype == 3)
			{             
			uint16_t fir = atoi(roww[6]);
			dbInfo.req_lenwoCRC =6+1+4;
		   // memcpy(&ik,&fir, sizeof(uint32_t));
		    dbInfo.buf[dbInfo.len++] = fir >> 8;
			dbInfo.buf[dbInfo.len++] = fir ;
			}
			if (dtype == 2)
			{
			int16_t fir = atoi(roww[6]);
			dbInfo.req_lenwoCRC =6+1+4;
		   // memcpy(&ik,&fir, sizeof(uint32_t));
		    dbInfo.buf[dbInfo.len++] = fir >> 8;
			dbInfo.buf[dbInfo.len++] = fir ;
			}
	
		}
		dbInfo.buf[4] = (dbInfo.len - 6) >> 8;
		dbInfo.buf[5] = (dbInfo.len - 6);
		vecWDbInfo.push_back(dbInfo);  
		tflag++;
	} 

	mysql_free_result(resultw);
	return 0;
}


int getalluser(vector <StCliInfo *> &vecCliInfo)
{
	MYSQL_RES *pMysql_res = NULL;
	char query[300];
	
	logDebug(log,"getalluser slq=%s\n", query);
	MYSQL mysql;
	ConnDatabase(mysql);
	MYSQL_ROW row;
	
	sprintf(query,"SELECT devname,ptype,retry,timeout, servip,servport,polltime,id from auth where active='1' and socktype='2'"); 
	logDebug(log,"slq11=%s\n", query);
	if (GetDataFromTable(mysql, query, pMysql_res))
	{
		logDebug(log,"getalluser: Call GetDataFromTable error%s", "");
		return -1;
	}

	while((row = mysql_fetch_row(pMysql_res))) 
	{               
		//StCliInfo *info = (StCliInfo *)malloc(sizeof(StCliInfo));
		StCliInfo *info = new StCliInfo;
		ConnDatabase(info->mysql);
		strcpy(info->user,row[0]);
		info->protocol = atoi(row[1]);
		info->retrynb = atoi(row[2]);
		info->tmout = atoi(row[3]);
		strcpy(info->ip,row[4]);
		info->port = atoi(row[5]);
		info->polltime = atoi(row[6]);
		info->id = atoi(row[7]);
		getChsInfo(mysql,info->user,info->vecChInfo);
		logDebug(log,"getalluser user is %s\n",info->user);
		logDebug(log,"getalluser protocol is %d\n",info->protocol);
		logDebug(log,"getalluser retrycount is %d\n",info->retrynb);
		logDebug(log,"getalluser timeout is %d\n",info->tmout);
		logDebug(log,"getalluser ip is %s\n",info->ip);
		logDebug(log,"getalluser port is %d\n",info->port);
		logDebug(log,"getalluser polltime is %d\n",info->polltime);
		logDebug(log,"getalluser id is %d\n",info->id);
		vecCliInfo.push_back(info);
	} 
	
	mysql_free_result(pMysql_res);
	mysql_close(&mysql);
	
	return 0;
}      

int getnewuser(vector <StCliInfo *> &vecCliInfo)
{
	MYSQL_RES *pMysql_res = NULL;
	char query[300];
	
	logDebug(log,"getalluser slq=%s\n", query);
	MYSQL mysql;
	ConnDatabase(mysql);
	MYSQL_ROW row;
	
	sprintf(query,"SELECT devname,ptype,retry,timeout, servip,servport,polltime,id from auth where active='1' and socktype='2' and threaded='0'"); 
	logDebug(log,"slq=%s\n", query);
	if (GetDataFromTable(mysql, query, pMysql_res))
	{
		logDebug(log,"getalluser: Call GetDataFromTable error%s", "");
		return -1;
	}

	while((row = mysql_fetch_row(pMysql_res))) 
	{               
		StCliInfo *info = new StCliInfo;
		ConnDatabase(info->mysql);
		strcpy(info->user,row[0]);
		info->protocol = atoi(row[1]);
		info->retrynb = atoi(row[2]);
		info->tmout = atoi(row[3]);
		strcpy(info->ip,row[4]);
		info->port = atoi(row[5]);
		info->polltime = atoi(row[6]);
		info->id = atoi(row[7]);
		getChsInfo(mysql,info->user,info->vecChInfo);
		logDebug(log,"getalluser user is %s\n",info->user);
		logDebug(log,"getalluser protocol is %d\n",info->protocol);
		logDebug(log,"getalluser retrycount is %d\n",info->retrynb);
		logDebug(log,"getalluser timeout is %d\n",info->tmout);
		logDebug(log,"getalluser ip is %s\n",info->ip);
		logDebug(log,"getalluser port is %d\n",info->port);
		logDebug(log,"getalluser polltime is %d\n",info->polltime);
		logDebug(log,"getalluser id is %d\n",info->id);
		vecCliInfo.push_back(info);
	} 
	
	mysql_free_result(pMysql_res);
	mysql_close(&mysql);
	
	return 0;
}      


int GetDeviceInfo(StCliInfo *cliInfo, vector <StWriteDbInfo> &vecCliInfo)
{
	char q[1024];
	sprintf(q,"SELECT ch,dtype,slaveid,funcode,startreg,countreg,history from %s WHERE active=('1')",cliInfo->user); 
	logDebug(log, "q=%s\n",q);
	if (mysql_query(&cliInfo->mysql, q)) 
	{
		logError(log, "%s\n", mysql_error(&cliInfo->mysql));
		return -1;
	}
	
	MYSQL_RES *result1 = mysql_store_result(&cliInfo->mysql);
	if (result1 == NULL) 
	{
		logError(log, "%s\n", mysql_error(&cliInfo->mysql));
		return -1;
	}

	MYSQL_ROW row1;

	StWriteDbInfo info;
	
	while((row1 = mysql_fetch_row(result1))) 
	{ 
		info.ch = atoi(row1[0]);
		info.datatype = atoi(row1[1]);
		info.slave_id = atoi(row1[2]);
		info.functioncode = atoi(row1[3]);
		int addr = atoi(row1[4])-1;
		info.nb = atoi(row1[5]);
		info.history = atoi(row1[6]);
		info.buf[0] = info.slave_id;
		info.buf[1] = info.functioncode;
		info.buf[2] =   addr >> 8;
		info.buf[3] = addr & 0x00ff;
		info.buf[4] =   info.nb  >> 8;
		info.buf[5] =  info.nb & 0x00ff;
		int rq_header_length = 2;
		int addr_length =2;
		int nb_length = 2;
		info.len =rq_header_length+addr_length+nb_length;
		
		uint16_t crc = crc16(info.buf, info.len);

		info.buf[info.len++] = crc >> 8;
		info.buf[info.len++] = crc & 0x00FF;
		vecCliInfo.push_back(info);
		//rb==length loop close                      
	}//while loop data send close
	
	mysql_free_result(result1);
	return 0;
}

int GetDeviceInfo2(StCliInfo *cliInfo, vector <StWriteDbInfo> &vecCliInfo, uint16_t &tflag)
{
	StWriteDbInfo info;
	char q[1024];
	sprintf(q,"SELECT ch,dtype,slaveid,funcode,startreg,countreg,history from %s WHERE active=('1')",cliInfo->user); 
	if (mysql_query(&cliInfo->mysql, q)) 
	{
		fprintf(stderr, "%s\n", mysql_error(&cliInfo->mysql));
		//printf("error 2");
		// mysql_close(con);
		//exit(1);
	}
	MYSQL_RES *result1 = mysql_store_result(&cliInfo->mysql);
	if (result1 == NULL) 
	{
		fprintf(stderr, "%s\n", mysql_error(&cliInfo->mysql));
		//printf("error 3");
	}
              
	MYSQL_ROW row1;
	               
	while((row1 = mysql_fetch_row(result1))) 
	{ 
		uint8_t mbtcpoffset = 6;
		
		info.ch = atoi(row1[0]);
		info.datatype = atoi(row1[1]);
		info.slave_id = atoi(row1[2]);
		info.functioncode = atoi(row1[3]);
		int addr = atoi(row1[4])-1;
		info.nb = atoi(row1[5]);
		info.history = atoi(row1[6]);
		
		info.buf[0] = tflag >> 8;
		info.buf[1] = tflag;
		info.buf[2] =0X00;
		info.buf[3] =0X00;
		info.buf[mbtcpoffset++] = info.slave_id;
		info.buf[mbtcpoffset++] = info.functioncode;
		info.buf[mbtcpoffset++] =   addr >> 8;
		info.buf[mbtcpoffset++] = addr & 0x00ff;
		info.buf[mbtcpoffset++] =   info.nb  >> 8;
		info.buf[mbtcpoffset++] =  info.nb & 0x00ff;
		int rq_header_length = 2;
        int addr_length =2;
        int nb_length = 2;
        info.len =(rq_header_length+addr_length+nb_length)+6;
		
		//int req_length =(rq_header_length+addr_length+nb_length)+6;

		//uint16_t crc = crc16(reqq, req_length);
		
		// reqq[req_length++] = crc >> 8;
		// reqq[req_length++] = crc & 0x00FF;
		info.buf[4] = (mbtcpoffset -6 ) >> 8;
		info.buf[5] = (mbtcpoffset -6 );
		 vecCliInfo.push_back(info);
		 tflag++;
	}//while loop data send close

	mysql_free_result(result1);  
	return 0;
}

int UpdateAlarmInfo(MYSQL &mysql, char *devname,int flag,int chid,int tagid, int alarmmore, float val,char *msg)
{
	char query[300];
	if (0 == flag)
	{
		sprintf(query,"update %s_alaconf set previousvalue=%d where chid=%d and tagid=%d",
			devname, flag, chid, tagid); 
					
	}
	else
	{
			sprintf(query,"update %s_alaconf set previousvalue=%d where chid=%d and tagid=%d",
			devname, flag, chid, tagid); 
					
	}
	
	logDebug(log, "UpdateAlarmInfo slq=%s\n", query);
	if (mysql_query(&mysql, query)) 
	{
		logError(log, "%s\n", mysql_error(&mysql));
		return -1;
	}

	if (1 == alarmmore)
	{
		char query2[300];
		sprintf(query2," INSERT INTO %s_alarm (alarm_name,alarmval) values ('%s',%f)",devname, msg,val);
		logDebug(log, "UpdateAlarmInfo query2 =%s \n", query2);
		if (mysql_query(&mysql, query2)) 
		{
			logError(log, "%s\n", mysql_error(&mysql));
		}
		else
		{
		    logDebug(log, "UpdateAlarmInfo  insert values in database\n");
		}
	}
	
	return 0;
}      

int IsStringEmpty(char *pBuf)
{
	if (NULL == pBuf)
	{
		return 1;
	}
	
	int nlen = strlen(pBuf);
	if (0 == nlen)
	{
		return 1;
	}

	return 0;
}


int updateAllAlalarminfo(MYSQL &mysql, vector<StAlarmInfo> vecalarm, char *devname,char *module)
{
	char query[300];
	sprintf(query,"SELECT chid,tagid,minvalue,maxval,previousvalue,alarmenable,alarmontext,alarmofftext from %s_alaconf",
		devname); 
	
	logDebug(log, "slq=%s\n", query);
	if (mysql_query(&mysql, query)) 
	{
		logError(log, "%s\n", mysql_error(&mysql));
		// mysql_close(con);
		return -1;
	}
	MYSQL_RES *result = mysql_store_result(&mysql);
	if (result == NULL) 
	{
		logError(log, "%s\n", mysql_error(&mysql));
		return -1;
	}
	
	mysql_num_fields(result);
	MYSQL_ROW row;
	StAlarmDetail alarmDetail;
	vector <StAlarmDetail> vecalarmDetail;
	string msginfo = module;
	while((row = mysql_fetch_row(result))) 
	{               
		memset(&alarmDetail, 0, sizeof(alarmDetail));
		/*logDebug(log, "updateAllAlalarminfo ch1=%s tag1=%s ch2=%s tag2=%s min=%s max=%s",
					row[0],
					row[1],
					row[2], row[3],
					row[4],row[5]);	*/
		
		if (!IsStringEmpty(row[0]))
			alarmDetail.chid = atoi(row[0]);
		if (!IsStringEmpty(row[1]))
			alarmDetail.tagid = atoi(row[1]);
		if (!IsStringEmpty(row[2]))
			alarmDetail.minvalue = atof(row[2]);
		if (!IsStringEmpty(row[3]))
			alarmDetail.maxval = atof(row[3]);
		if (!IsStringEmpty(row[4]))
			alarmDetail.previousvalue = atoi(row[4]);
		if (!IsStringEmpty(row[5]))
			alarmDetail.alarmenable = atoi(row[5]);

		if (!IsStringEmpty(row[6]))
		snprintf(alarmDetail.alarmontext,
			sizeof(alarmDetail.alarmontext),
			"%s", row[6]);
		if (!IsStringEmpty(row[7]))
			snprintf(alarmDetail.alarmofftext,
			sizeof(alarmDetail.alarmofftext),
			"%s", row[7]);
		
		vecalarmDetail.push_back(alarmDetail);
	} 
	
	mysql_free_result(result);
	int writelog = 0;
	int alarmmore = 0;
	int find = 0;
	int previousvalue = 0;
	
	for (size_t i=0; i<vecalarm.size(); i++)
	{
		writelog = 0;
		find = 0;
		alarmmore = 0;
		for (size_t j=0; j<vecalarmDetail.size(); j++)
		{
			if (vecalarm[i].chid == vecalarmDetail[j].chid
				&& vecalarm[i].tagid == vecalarmDetail[j].tagid)
			{
				if (vecalarmDetail[j].alarmenable == 1
				&& (vecalarm[i].val < vecalarmDetail[j].minvalue || vecalarm[i].val > vecalarmDetail[j].maxval))
				{
					writelog = 1;
					msginfo = vecalarmDetail[j].alarmontext;
				}
				else
				{
					msginfo = vecalarmDetail[j].alarmofftext;	
				}

				char tmp[24];
				snprintf(tmp, sizeof(tmp), "%d%d",
					vecalarm[i].chid, vecalarm[i].tagid);
				previousvalue = vecalarmDetail[j].previousvalue;
				if (g_mapdata.find(tmp) == g_mapdata.end())
				{
					g_mapdata[tmp] = tmp;
					previousvalue = 0;
				}
				
				find = 1;
				logDebug(log, "finddata*****ch1=%d tag1=%d ch2=%d tag2=%d min=%f max=%f val=%f module=%s",
					vecalarm[i].chid,vecalarm[i].tagid,
					vecalarmDetail[j].chid,vecalarmDetail[j].tagid,
					vecalarmDetail[j].minvalue, vecalarmDetail[j].maxval,
					vecalarm[i].val, module);	
				break;
				
			}
			else
			{
				logDebug(log, "cant find*****ch1=%d tag1=%d ch2=%d tag2=%d",
					vecalarm[i].chid,vecalarm[i].tagid,
					vecalarmDetail[j].chid,vecalarmDetail[j].tagid);	
			}
		}

		if (0 == find)
		{
			continue;
		}
		
		if ((writelog == 1 && previousvalue == 0)
			|| (writelog == 0 && previousvalue == 1))
			alarmmore = 1;
		
		UpdateAlarmInfo(mysql, devname, writelog, vecalarm[i].chid, vecalarm[i].tagid, alarmmore, 
			vecalarm[i].val,(char *)msginfo.c_str());
		
	}

	
	
	return 0;	
}


int store_result_coil(StCliInfo &cliInfo,int ch,int nb,int res_length,char bbuuff[],int proto,int history)
{
	int i;
	uint8_t rsp_io_byte[32];
	int calc_length = res_length -5;
	int k=0;
	int j=0;
	char bits[256];
	int offset;
	if(proto == 1)
	{
		offset = 0;
	}
	
	if(proto == 2)
	{
		offset = 6;
	}

	for(i=0;i<calc_length;i++)
	{                      
		int mask =1;
		rsp_io_byte[i] = bbuuff[i+offset+3];
		// printf("bbuuff<%.2X> \n",rsp_io_byte[i] );
		for(k=j;k<j+8;k++)
		{
			bits[k] = rsp_io_byte[i] >> (k-(i*8)) & mask;
		//  printf("bits[%d]%d \n",k,bits[k]);
		} //for loop of K close
		
		j=k;
	}// for loop of i close

	bool write = 0;
	for (size_t  i=0; i<cliInfo.vecChInfo.size(); i++)
	{
		if (ch == cliInfo.vecChInfo[i].chid)
		{
			write = 1;
			break;
		}
			
	}

	StAlarmInfo alarminfo;
	vector<StAlarmInfo> vecalarm;


if(history == 1)
	{	

	int lastid;
	char query2[300];
	int n;
	sprintf(query2," INSERT INTO %s_%d (channel,data_1) values ('%d','%d')",cliInfo.user,ch,ch,bits[0]);

	
	logDebug(log, "store_result_coil query2 =%s \n", query2);
	if (mysql_query(&cliInfo.mysql, query2)) 
	{
		logError(log, "%s\n", mysql_error(&cliInfo.mysql));
		//mysql_close(con);
		//exit(1);
	}
	else
	{
	    logDebug(log, "store_result_coil success insert values in database\n");
	    lastid = mysql_insert_id(&cliInfo.mysql);
	    logDebug(log, "store_result_coil last insert id is %d\n",lastid);
	}
	
	for(n=0;n<nb;n++)
	{
		char query3[300];char query4[200];
		sprintf(query3," UPDATE %s_%d set data_%d = ('%d') where id=%d",cliInfo.user,ch,n+1,bits[n],lastid);
		sprintf(query4," UPDATE %s_livedata_%d set data_%d = ('%d') where id=%d",cliInfo.user,ch,n+1,bits[n],1);
			if (1 == write)
			{
				alarminfo.chid = ch;
				alarminfo.tagid = n+1;
				alarminfo.val = bits[n];
				vecalarm.push_back(alarminfo);	
			}
		
		logDebug(log, "store_result_coil query3 =%s nb=%d\n", query3, nb);
		if (mysql_query(&cliInfo.mysql, query3)) 
		{
			logError(log, "%s\n", mysql_error(&cliInfo.mysql));
			//mysql_close(con);
			//exit(1);
		} //mysql query update close
		if (mysql_query(&cliInfo.mysql, query4)) 
		{
			logError(log, "%s\n", mysql_error(&cliInfo.mysql));
			//mysql_close(con);
			//exit(1);
		} //mysql query update close
	 
	} //n close   
	}
else{
	
	int n;

	for(n=0;n<nb;n++)
	{
		char query4[200];
		
		sprintf(query4," UPDATE %s_livedata_%d set data_%d = ('%d') where id=%d",cliInfo.user,ch,n+1,bits[n],1);
		if (1 == write)
			{
				alarminfo.chid = ch;
				alarminfo.tagid = n+1;
				alarminfo.val = bits[n];
				vecalarm.push_back(alarminfo);
			}
		
		
		logDebug(log, "store_result_coil query3 =%s nb=%d\n", query4, nb);
		
		if (mysql_query(&cliInfo.mysql, query4)) 
		{
			logError(log, "%s\n", mysql_error(&cliInfo.mysql));
			//mysql_close(con);
			//exit(1);
		} //mysql query update close
	 
	} //n close   

	

	}   

	updateAllAlalarminfo(cliInfo.mysql, vecalarm,cliInfo.user,"store_result_coil");

	return 0;
}//function close



int store_result_reg(StCliInfo &cliInfo, int ch,int nb,int datatype,char bbuuff[],int proto,int history)
{
	uint16_t tab_registers[128];
	float f[128];
	int floatoffset =0;
	int i=0;
	// printf("\n value of Nb is %d \n\n\n",nb);      
	int offset;
	for (i=0;i<nb;i++)
	{
		if(proto == 1){offset = (i*2);}
		if(proto == 2){offset = ((i*2)+6);}

    tab_registers[i] = (unsigned char)bbuuff[offset+3] << 8 | (unsigned char)bbuuff[offset+4];
 //   printf("bbuuff<%.2X>", (unsigned char)bbuuff[offset+3]);
 //   printf("bbuuff<%.2X>", (unsigned char)bbuuff[offset+4]);
  //  printf("\n tab register<%d> value is %.4X \n",i,tab_registers[i]);
	i=i+1;
	//offset =i*2; 
	if(proto == 1)
	{offset = (i*2);}
	if(proto == 2){offset = ((i*2)+6);}   
	//  printf("bbuuff<%.2X>", (unsigned char)bbuuff[offset+3]);
	//  printf("bbuuff<%.2X>", (unsigned char)bbuuff[offset+4]);    
	tab_registers[i] = (unsigned char)bbuuff[offset+3] << 8 | (unsigned char)bbuuff[offset+4];
	//   printf("\n tab register<%d> value is %.4X \n",i,tab_registers[i]);
	if(datatype == 4) 
	{
		f[floatoffset]= modbus_get_float(tab_registers[i-1],tab_registers[i]);
	}
	if(datatype == 5) 
	{
		f[floatoffset]= modbus_get_float_badc(tab_registers[i-1],tab_registers[i]);    
	}

	if(datatype == 6) 
	{
		f[floatoffset]= modbus_get_float_abcd(tab_registers[i-1],tab_registers[i]);
	}


	if(datatype == 7) 
	{
		f[floatoffset]= modbus_get_float_dcba(tab_registers[i-1],tab_registers[i]);
	}
	if(datatype == 8) 
	{
		f[floatoffset]= modbus_get_float_cdab(tab_registers[i-1],tab_registers[i]);
	}        
	//   printf("\n tab register<%d> value is %.4X%.4X \n",i,tab_registers[i-1],tab_registers[i]);
	//  printf("f<%d>float value is %f \n",floatoffset,f[floatoffset]);
	floatoffset++;

	}

	bool write = 0;
	for (size_t i=0; i<cliInfo.vecChInfo.size(); i++)
	{
		if (ch == cliInfo.vecChInfo[i].chid)
		{
			write = 1;
			break;
		}
			
	}

	StAlarmInfo alarminfo;
	vector<StAlarmInfo> vecalarm;

if(history == 1)
	{	
	int k=0;
	int lastid;
	char query2[300];
	memset(&alarminfo, 0, sizeof(alarminfo));
	sprintf(query2," INSERT INTO %s_%d (channel,data_1) values ('%d','%f')",cliInfo.user,ch,ch,f[0]);

	//  printf("%s",query2);
	logDebug(log, "store_result_reg query2 =%s \n", query2);
	if (mysql_query(&cliInfo.mysql, query2)) 
	{
		logError(log, "%s\n", mysql_error(&cliInfo.mysql));
		
	//mysql_close(con);
	//exit(1);
	}
	else
	{
		logDebug(log, "store_result_reg success insert values in database\n");
		lastid = mysql_insert_id(&cliInfo.mysql);
		logDebug(log, "store_result_reg last insert id is %d\n",lastid);
	}
	
	for(k=0;k<(nb/2);k++)
	{
		char query3[300];char query4[200];
		sprintf(query3," UPDATE %s_%d set data_%d = %f where id=%d",cliInfo.user,ch,(k*2)+1,f[k],lastid);
		sprintf(query4," UPDATE %s_livedata_%d set data_%d = %f  where id=%d",cliInfo.user,ch,(k*2)+1,f[k],1);
		if (1 == write)
			{
				alarminfo.chid = ch;
				alarminfo.tagid = (k*2)+1;
				alarminfo.val = f[k];
				vecalarm.push_back(alarminfo);
			}
		
		logDebug(log, "store_result_reg query3 =%s nb=%d\n", query3, nb);
		if (mysql_query(&cliInfo.mysql, query3)) 
	    {
	    	logError(log, "%s\n", mysql_error(&cliInfo.mysql));
	    	//mysql_close(con);
	    	//exit(1);
	    }
	    if (mysql_query(&cliInfo.mysql, query4)) 
	    {
	    	logError(log, "%s\n", mysql_error(&cliInfo.mysql));
	    	//mysql_close(con);
	    	//exit(1);
	    }

	}
}
else{
	int k=0;
	
	for(k=0;k<(nb/2);k++)
	{
		char query4[200];
		sprintf(query4," UPDATE %s_livedata_%d set data_%d = %f where id=%d",cliInfo.user,ch,(k*2)+1,f[k],1);
		logDebug(log, "store_result_reg query4 =%s nb=%d\n", query4, nb);
		if (1 == write)
	{
		alarminfo.chid = ch;
		alarminfo.tagid = (k*2)+1;
		alarminfo.val = f[k];
		vecalarm.push_back(alarminfo);
	}
		
	    if (mysql_query(&cliInfo.mysql, query4)) 
	    {
	    	logError(log, "%s\n", mysql_error(&cliInfo.mysql));
	    	//mysql_close(con);
	    	//exit(1);
	    }

	}


	}

	updateAllAlalarminfo(cliInfo.mysql, vecalarm,cliInfo.user,"store_result_reg");

	return 0;
}//function close

int store_result_holding16(StCliInfo &cliInfo,int ch,int nb,int datatype,char bbuuff[],int proto,int history)
{
	uint16_t tab_registers[128];
	int f[128];
	int offset =0;
	int i=0;
	int intoffset= 0;
	// printf("\n value of Nb is %d \n\n\n",nb);      
	
	for (i=0;i<nb;i++)
	{
		
  //  tab_registers[i] = (unsigned char)bbuuff[offset+4] << 8 | (unsigned char)bbuuff[offset+3];
   // printf("bbuuff<%.2X>", (unsigned char)bbuuff[offset+3]);
   // printf("bbuuff<%.2X>", (unsigned char)bbuuff[offset+4]);
  //  printf("\n tab register<%d> value is %.4X \n",i,tab_registers[i]);
	
	 
	if(proto == 1)
	{offset = i*2;}
	if(proto == 2){offset = (i*2)+6;}   
	//  printf("bbuuff<%.2X>", (unsigned char)bbuuff[offset+3]);
	 // printf("bbuuff<%.2X>", (unsigned char)bbuuff[offset+4]);    
	tab_registers[i] = (unsigned char)bbuuff[offset+4] << 8 | (unsigned char)bbuuff[offset+3];
	//   printf("\n tab register<%d> value is %.4X \n",i,tab_registers[i]);
	if(datatype == 2) 
	{
		f[i]= modbus_get_16hextoint(tab_registers[i]);
	}	
	
	if(datatype == 3) 
	{
		f[i]= modbus_get_unsign_16hextoint(tab_registers[i]);
		
	}
	//offset=offset+2;
	//i=i+1;
	intoffset++;

	}

	StAlarmInfo alarminfo;
	vector<StAlarmInfo> vecalarm;
	bool write = 0;
	for (int i=0; i<cliInfo.vecChInfo.size(); i++)
	{
		if (ch == cliInfo.vecChInfo[i].chid)
		{
			write = 1;
			break;
		}
			
	}

	if(history == 1)
	{	
	int k=0;
	int lastid;
	char query2[300];
	sprintf(query2," INSERT INTO %s_%d (channel,data_1) values ('%d','%d')",cliInfo.user,ch,ch,f[0]);
	
	//  printf("%s",query2);
	logDebug(log, "store_result_holding16 query2 =%s \n", query2);
	if (mysql_query(&cliInfo.mysql, query2)) 
	{
		logError(log, "%s\n", mysql_error(&cliInfo.mysql));
		
	//mysql_close(con);
	//exit(1);
	}
	else
	{
		logDebug(log, "store_result_holding16 success insert values in database\n");
		lastid = mysql_insert_id(&cliInfo.mysql);
		logDebug(log, "store_result_holding16 last insert id is %d\n",lastid);
	}
	
	for(k=0;k<nb;k++)
	{
		char query3[300]; char query4[200];
		sprintf(query3," UPDATE %s_%d set data_%d = %d where id=%d",cliInfo.user,ch,k+1,f[k],lastid);
		if (1 == write)
	{
		alarminfo.chid = ch;
		alarminfo.tagid = k+1;
		alarminfo.val = f[k];
		vecalarm.push_back(alarminfo);
	}
		
		sprintf(query4," UPDATE %s_livedata_%d set data_%d = %d where id=%d",cliInfo.user,ch,k+1,f[k],lastid);
		
		logDebug(log, "store_result_holding16 query3 =%s nb=%d\n", query3, nb);
		if (mysql_query(&cliInfo.mysql, query3)) 
	    {
	    	logError(log, "%s\n", mysql_error(&cliInfo.mysql));
	    	//mysql_close(con);
	    	//exit(1);
	    }
	    if (mysql_query(&cliInfo.mysql, query4)) 
	    {
	    	logError(log, "%s\n", mysql_error(&cliInfo.mysql));
	    	//mysql_close(con);
	    	//exit(1)
		}
	}
	}	
else{
	int k=0;

	for(k=0;k<nb;k++)
	{
		char query4[200]; 
		
		sprintf(query4," UPDATE %s_livedata_%d set data_%d = %d where id=%d",cliInfo.user,ch,k+1,f[k],1);
		if (1 == write)
	{
		alarminfo.chid = ch;
		alarminfo.tagid = k+1;
		alarminfo.val = f[k];
		vecalarm.push_back(alarminfo);
	}
		
		logDebug(log, "store_result_holding16 query4 =%s nb=%d\n", query4, nb);
		if (mysql_query(&cliInfo.mysql, query4)) 
	    {
	    	logError(log, "%s\n", mysql_error(&cliInfo.mysql));
	    	//mysql_close(con);
	    	//exit(1);
	    }
	}    
}	

	updateAllAlalarminfo(cliInfo.mysql, vecalarm,cliInfo.user,"store_result_holding16");

	return 0;
}//function close
