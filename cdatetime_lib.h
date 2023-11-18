/***************************************
  *   版权所有(C) 2003-2005.
  *   文 件 名 : lib_cdatetime.h
  *   作    者 : 
  *   完成日期 :
  *   描    述 : 时间类头文件。
  *   修改记录 :
  *   数据结构 :
  *   备    注 :
  ***************************************/

#ifndef __LIB_CDATETIME_H
#define __LIB_CDATETIME_H


#include <cstring>
#include <iostream>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <unistd.h>

using namespace std;


//#ifdef __cplusplus
//extern "C"
//{
//#endif


// date buffer len  
const int g_date_len = 11;
// time buffer len  
const int g_time_len = 9;

// mitime buffer len  
const int g_mitime_len = 20;

class CDateTime
{
public:
    CDateTime() {};
    ~CDateTime() {};

    // 取当前系统日期时间  
     char*  getDateTime();    

    // 取当前系统日期 
     char*  getDate();

    // 取当前系统时间 
     char*  getTime();

	// 取当前系统时间毫秒
     char*  getMiTime();

private:
    char m_datetime[g_date_len + g_time_len];
    char m_date[g_date_len]; 
    char m_time[g_time_len];
    char m_mitime[g_mitime_len];    
};

//#ifdef __cplusplus
//}
//#endif

#endif // __CDATETIME_H
