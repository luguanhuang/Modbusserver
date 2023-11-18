
#ifndef SERVICE_THREAD_POOL_H
#define SERVICE_THREAD_POOL_H
#include <pthread.h>
#include <vector>
using namespace std;


#include "log_lib_cplus.h"

typedef struct
{
	int chid;
	char chdesc[64];
}StChInfo;

typedef struct
{	
	vector <StChInfo> vecChInfo;
	int id;
	char user[50];
	char ip[32];
	int protocol;
	int retrynb; 
	int tmout;
	int polltime;	
	int newsocket;
	int port;
	char buffer[256];
	MYSQL mysql;
	
}StCliInfo;

typedef struct
{	
	uint8_t buf[256];
	uint8_t len;
	int id;
	int nb;
	int ch;
	int datatype;
	int slave_id;
	int functioncode;
	int req_lenwoCRC;
	int history;
}StWriteDbInfo;

int StartThread(StCliInfo *cliInfo, int type);
void *ConnectServer(void *pParam);
void *MaintainThread(void *pParam);

#endif
