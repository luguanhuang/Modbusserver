
#ifndef MBW_H_H
#define MBW_H_H
#include <mysql.h>
#include "mbw_thread.h"
#include <vector>
using namespace std;

typedef struct
{
	int chid;
	int tagid;
	double val;
}StAlarmInfo;

typedef struct
{
	int chid;
	int tagid;
	float minvalue;
	float maxval;
	int previousvalue;
	int alarmenable;
	char alarmontext[64];
	char alarmofftext[64];
}StAlarmDetail;

int getmbwriteinfo(StCliInfo *cliInfo, vector <StWriteDbInfo> &vecWDbInfo);
int GetDeviceInfo(StCliInfo *cliInfo, vector <StWriteDbInfo> &vecCliInfo);
int ConnDatabase(MYSQL &mysql);

int getuser(char buff[], StCliInfo *clientInfo, int &cnt);
int store_result_reg(StCliInfo &cliInfo, int ch,int nb,int datatype,char bbuuff[],int proto,int history);
//int store_result_coil(MYSQL &mysql, char user[],int ch,int nb,int dtype,int res_length,char bbuuff[],int proto);
int store_result_coil(StCliInfo &cliInfo,int ch,int nb,int res_length,char bbuuff[],int proto,int history);
int getalluser(vector <StCliInfo *> &vecCliInfo);
int updatembinfo(StCliInfo *cliInfo, int id);
int updatechtime(StCliInfo *cliInfo, int chid);
int getmbwriteinfo2(StCliInfo *cliInfo, vector <StWriteDbInfo> &vecWDbInfo, uint16_t &tflag);
int GetDeviceInfo2(StCliInfo *cliInfo, vector <StWriteDbInfo> &vecCliInfo, uint16_t &tflag);
int UpdateConnectStatus(MYSQL &mysql, int id,int status);
int store_result_holding16(StCliInfo &cliInfo,int ch,int nb,int datatype,char bbuuff[],int proto,int history);
int getnewuser(vector <StCliInfo *> &vecCliInfo);
int UpdateCloseConnStatus(MYSQL &mysql, int id,int status);
int resetdevicestatus();
#endif
