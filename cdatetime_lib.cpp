/*******************************************************************************
  *   版权所有(C) 2003-2005.
  *   文 件 名 : lib_cdatetime.cpp
  *   作    者 : 
  *   完成日期 :  
  *   描    述 : 日志类例程。
  *   修改记录 :
  *   数据结构 :
  *   备    注 :
  ******************************************************************************/


#include "cdatetime_lib.h"

/*******************************************************************************
  *   版权所有(C) 2003-2005.
  *   函 数 名 : getDateTime
  *   用    途 : 日期类取当前系统日期时间函数。
  *   参    数 : 无
  *   返 回 值 : const char* const
  *   文 件 名 :  
  *   作    者 : 
  *   完成日期 :  
  *   描    述 :
  *   修改记录 :
  *   数据结构 : 
  *   备    注 :日期时间格式为：YYYYMMDDHHMMSS
  ******************************************************************************/
char*  CDateTime::getDateTime()
{
	struct tm *ptm;
	time_t t = 0;
	
	memset(m_datetime, 0x00, g_date_len + g_time_len);
	
	// convert time format  
	time(&t);
	ptm = localtime(&t);
	
	strftime(m_datetime, g_date_len + g_time_len, "%Y%m%d%H%M%S", ptm);
	
	return m_datetime;
}

/*******************************************************************************
  *   版权所有(C) 2003-2005, .
  *   函 数 名 : getDate
  *   用    途 : 日期类取当前系统日期函数。
  *   参    数 : 无
  *   返 回 值 : const char* const
  *   文 件 名 : clog.cpp
  *   作    者 : 
  *   完成日期 : 2003年 07月 21日
  *   描    述 :
  *   修改记录 :
  *   数据结构 : 
  *   备    注 :日期格式为：YYYY-MM-DD
  ******************************************************************************/
 char*  CDateTime::getDate()
{
	struct tm *ptm;
	time_t t = 0;
	
	memset(m_date, 0x00, g_date_len);
	
	// convert time format  
	time(&t);
	ptm = localtime(&t);
	
	strftime(m_date, g_date_len, "%Y-%m-%d", ptm);
	
	return m_date;
}

/*******************************************************************************
  *   版权所有(C) 2003-2005, .
  *   函 数 名 : getTime
  *   用    途 : 日期类取当前系统时间函数。
  *   参    数 : 无
  *   返 回 值 : const char* const
  *   文 件 名 : clog.cpp
  *   作    者 :  
  *   完成日期 :  
  *   描    述 :
  *   修改记录 :
  *   数据结构 : 
  *   备    注 :时间格式为：HH:MM:SS
  ******************************************************************************/
 char*  CDateTime::getTime()
{
	struct tm *ptm;
	time_t t = 0;

	memset(m_time, 0x00, g_time_len);
	
	// convert time format  
	time(&t);
	ptm = localtime(&t);
	
	strftime(m_time, g_time_len, "%X", ptm);
	
	return m_time;
}


/*******************************************************************************
  *   版权所有(C) 2003-2005, .
  *   函 数 名 : getMiTime
  *   用    途 : 日期类取当前系统时间函数。
  *   参    数 : 无
  *   返 回 值 : const char* const
  *   文 件 名 :  
  *   作    者 :  
  *   完成日期 :  
  *   描    述 :
  *   修改记录 :
  *   数据结构 : 
  *   备    注 :时间格式为： HH:MM:SS,UUU
  ******************************************************************************/
 char*  CDateTime::getMiTime()
{
	struct tm *ptm;
    struct timeb tp;
	char strtime[g_time_len];

	ftime(&tp);
	
	memset (strtime, 0x0, sizeof(strtime));
	memset (m_mitime, 0x0, g_mitime_len);
	
	// convert time format  
	ptm = localtime(&(tp.time));
	
	strftime(strtime, sizeof(strtime), "%X", ptm);
	snprintf(m_mitime, sizeof(m_mitime)-1, "%s,%03d", strtime, tp.millitm);

        return m_mitime;
}


