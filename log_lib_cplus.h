/***************************************
  *   版权所有(C) 2003-2005.
  *   文 件 名 : _lib_clog.h
  *   作    者 : 
  *   完成日期 : 
  *   描    述 : 日志类头文件。
  *   修改记录 :
  *     作者    时间        版本    描述
  *   数据结构 :
  *   备    注 :
  ***************************************/

#ifndef __LIB_CLOG_CPLUS_H__
#define __LIB_CLOG_CPLUS_H__


#include <string>
#include <map>
#include <cstring>
#include <iostream>

//#ifdef __cplusplus
//extern "C" {
//#endif // __cplusplus 
 
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>

//#ifdef __cplusplus
//}
//#endif // __cplusplus 


#include "cdatetime_lib.h"


using namespace std;


#ifndef MAX_FILENAME_LEN
#define MAX_FILENAME_LEN    256     // 文件名的最大长度         
#endif

// 日志文件默认大小 
#define CLOG_MAX_FILESIZE   (1024*1024)


// 日志等级  
enum {
    CLOG_DEBUG = 0,
    CLOG_INFO,
    CLOG_ERROR,
    CLOG_NONE
};


class CLog
{
public:
    CLog();
    CLog(const char *szPath);
    CLog(const string& cstrPath);
    ~CLog();
   
    // 日志环境初始化 
    int init(int iloglevel = CLOG_DEBUG);

    // 日志环境初始化 
    int init(const string& cstrPath, int iloglevel = CLOG_DEBUG);

    // 更改日志文件的名称 
    int rename(const char *fname);

    // 运行日志写函数 
    int write(const char *fmt, ...);

    // 调试日志写函数 
    int write(const char *file, const char *func, int line, const char *fmt, ...);

    // 根据日志级别写日志的函数 
    int write(int lvl, const char *file, const char *func, int line, const char *fmt, ...);
//	int write(int lvl, const char *fmt, ...);

    // 设置日志级别的函数 
    int setLogLevel(int lvl);

    // 设置日志级别的函数 
    int setLogLevel(const char* szLvl);

    // 获取日志级别的函数 
    int getLogLevel(void);

    // 获取当前日志级别名称的函数 
    const char* getLogLevelName(void);

    // 获取指定日志级别名称的函数 
    const char* getLogLevelName(int lvl);

    // 设置日志文件的最大尺寸 
    void setMaxFileSize(int iMaxSize){ m_maxFileSize = iMaxSize; };

    // 读取日志文件的最大尺寸 
    int getMaxFileSize(){ return m_maxFileSize; };

    // 清理资源，关闭已打开的文件 
    void cleanup(void);

    int writeBuffer(const char *file, const char *func, const char* buffer, const int len);

    // SOC 日志 
 //   CSOCLog soc;

private:
    // 设置初始化日志环境所需要的资源 
    void fill(const string& cstrPath);

    // 检测日志文件的大小是否超限, 如果超限就自动转存 
    int chkLogFileSize(void);

    // 打开日志文件 
    int openLog();

    // 关闭日志文件 
    int closeLog();

private:
    static map<string, int> ms_filemap;

    FILE    *m_logfp;

    CDateTime   m_datetime;     // datetime utils 
    std::string m_logfname;     // 日志文件的全路径名 
    std::string m_logfpath;     // 日志文件存放的路径 
    pthread_mutex_t   m_mutex;  // 日志文件锁 

    int  m_maxFileSize; // 日志文件的最大尺寸 
    int  m_logLevel;    // 当前配置的日志级别 

    int  m_flag;  // 构造函数是否执行成功 
};


#define logError(clog, fmt, arg...) \
do \
{ \
    clog.write(CLOG_ERROR, __FILE__, __FUNCTION__, __LINE__, fmt, ##arg); \
} while (0)


#define logDebug(clog, fmt, arg...) \
do \
{ \
    clog.write(CLOG_DEBUG, __FILE__, __FUNCTION__, __LINE__, fmt, ##arg); \
} while (0)


#endif // __CLOG_H__ 

/* 
 * vi: sw=4 ts=4 et
*/ 

/*******************************************************************************

#include "lib_log.h"

int main(int argc, char *argv[])
{
    string szLogFile="./log";        //不需要后缀默认是 .log
    CLog log(szLogFile);   
    if (log.init(CLOG_DEBUG) < 0)
    {
        cout<<"init log faild."<<endl;
        exit(-1);
    }
    logDebug(log, "hello.");
    logError(log, "sorry.");
}
*******************************************************************************/

