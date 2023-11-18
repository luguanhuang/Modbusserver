/******************************************************************************
  *   版权所有(C) 2003-2050
  *   文 件 名 : 
  *   作    者 : 
  *   完成日期 : 
  *   描    述 : 日志类例程。
  *   修改记录 :
  *     作者    时间        版本    描述
  *    在获取日志的等级配置时, 采用更安全的方式获取日志等级配置信息
  *    修改日志中与环境变量相关的部分, 提高类的内聚特性
  *    日志类改为单一日志文件记录
  *    增加rename()函数, 用于在运行过程中更改日志文件名 
  *    修改日志格式为 "(pid) [date time] [file:line] [function]"
  *    日志的分级改为由clog类中的日志级别来决定
  *    增加新的带有日志级别的write()接口用于输出分级日志 
  *    增加了对日志级别操作的函数 
  *   数据结构 :
  *   备    注 :
  *****************************************************************************/

#include "log_lib_cplus.h"

// 记录到日志中的日志等级标志  
static const char * s_lvlname[CLOG_NONE+1] = {
    "DBG",
    "INF",
    "ERR",
    "NONE"
};

// 保存同名日志文件的map 
map<string,int> CLog::ms_filemap;

// 构造函数 
CLog::CLog()
{
    m_flag  = 0;
    m_logfp = NULL;
    m_logLevel = CLOG_DEBUG;     // 日志的默认等级 
    m_logfname = "";
    m_logfpath = "./";
    m_maxFileSize = CLOG_MAX_FILESIZE; // 日志文件的最大尺寸 
    pthread_mutex_init(&m_mutex, NULL);
};


// 构造函数 
CLog::CLog(const string& cstrPath)
{
    m_flag  = 0;
    m_logfp = NULL;

    m_logLevel = CLOG_DEBUG;     // 日志的默认等级 

    m_logfname = "";
    m_logfpath = "./";
    m_maxFileSize = CLOG_MAX_FILESIZE; // 日志文件的最大尺寸 

    pthread_mutex_init(&m_mutex, NULL);
    fill(cstrPath);
}


// 构造函数 
CLog::CLog(const char* szPath)
{
    m_flag  = 0;
    m_logfp = NULL;

    m_logLevel = CLOG_DEBUG;     // 日志的默认等级 

    m_logfname = "";
    m_logfpath = "./";
    m_maxFileSize = CLOG_MAX_FILESIZE; // 日志文件的最大尺寸 

    string cstrPath = "";
    if (szPath)
    {
        cstrPath = szPath;
    }

    pthread_mutex_init(&m_mutex, NULL);
	pthread_mutex_lock(&m_mutex);
    fill(cstrPath);
	pthread_mutex_unlock(&m_mutex);
}


// 析构函数 
CLog::~CLog()
{
    cleanup();
}


/*****************************************************************************
 *   版权所有(C) 2003-2050.
 *   函 数 名 : init
 *   用    途 : 日志类日志系统初始化函数。
 *   参    数 : const char* pmode
 *   返 回 值 : int 0表示成功，-1表示失败
 *   文 件 名 : 
 *   作    者 : 
 *   完成日期 : 
 *   描    述 :
 *   修改记录 :
 *   数据结构 : 
 *   备    注 :
 *****************************************************************************/
//int CLog::init(int iloglevel = CLOG_DEBUG);
int CLog::init(int iloglevel)
{
    if ( m_flag )
    {
        return -1;
    }

    setLogLevel(iloglevel);
    
    return 0;
}


//int init(const string& cstrPath, int iloglevel = CLOG_DEBUG);
int CLog::init(const string& cstrPath, int iloglevel)
{
    // 清除原有的资源 
    cleanup();
    
    // 设置基础信息 
    fill(cstrPath);
    if (m_flag != 0)
    {
        return -1;
    }

    return init(iloglevel);
}


/*****************************************************************************
 *   函 数 名 : write
 *   用    途 : 日志类运行日志写函数。
 *   参    数 : const char* fmt
 *      ...
 *   返 回 值 : int 0表示成功，-1表示失败
 *   文 件 名 : clog.cpp
 *   作    者 : 
 *   完成日期 : 
 *   描    述 :
 *   修改记录 :
 *   数据结构 : 
 *   备    注 :
*****************************************************************************/
int CLog::write(const char* fmt, ...)
{
    int ret = 0;

    if (0 != (ret = openLog()))
    {
        return ret;
    }
    
    if (0 != (ret = chkLogFileSize()) )
    {
        closeLog();
        return ret;
    }

	pthread_t thread_id = pthread_self();
    // base info 
    fprintf(m_logfp, "(t%d) [%s %s]\n\t", 
                     thread_id, 
                     m_datetime.getDate(), m_datetime.getMiTime());
    
    // run info 
    va_list pvar;
    va_start(pvar, fmt);    
    vfprintf(m_logfp, fmt, pvar);
    va_end(pvar);

    fprintf(m_logfp, "\n");
    
    return closeLog();
}


/*****************************************************************************
  *   函 数 名 : write
  *   用    途 : 日志类调试日志写函数。
  *   参    数 : const char* file
  *     const char* func
  *     const int line
  *     const char* fmt
  *       ...
  *   返 回 值 : int 0表示成功，-1表示失败
  *   文 件 名 : clog.cpp
  *   作    者 : 
  *   完成日期 : 
  *   描    述 :
  *   修改记录 :
  *   数据结构 : 
  *   备    注 :
  *****************************************************************************/
int CLog::write(const char* file, const char* func, const int line, const char* fmt, ...) 
//int CLog::write(const char* fmt, ...) 
{
    if (!file || !func)
    {
        return -1;
    }

    int ret = 0;

    if (0 != (ret = openLog()))
    {
        return ret;
    }
    
    if (0 != (ret = chkLogFileSize()) )
    {
        closeLog();
        return ret;
    }

	pthread_t thread_id = pthread_self();
    fprintf(m_logfp, "(t%d) [%s %s]\n\t", 
                     thread_id, 
                     m_datetime.getDate(), m_datetime.getMiTime(),
                     file, line, func);

    va_list ap;
    va_start(ap, fmt);    
    vfprintf(m_logfp, fmt, ap);
    va_end(ap);
    
    fprintf(m_logfp, "\n");
    
    return closeLog();
}

// 清理资源，关闭已打开的文件 
void CLog::cleanup(void)
{
    ;
}

/*****************************************************************************
  *   用    途 : 设置初始化日志环境所需要的资源
  *   参    数 : cstrPath   日志文件的全路径 
  *   返 回 值 : 无 
  *   文 件 名 : clog.cpp
  *   作    者 : 
  *   完成日期 : 
  *   描    述 :
  *   修改记录 :
  *   数据结构 : 
  *   备    注 :
  ***************************************************************************/
void CLog::fill(const string& cstrPath)
{
    m_flag = -1;
	//modify by luguanhuang 20180905
	//多线程调用会挂掉 先注掉
#if 0
    map<string,int>::iterator pos = ms_filemap.find(cstrPath);

    int filecount;
    if (pos != ms_filemap.end())
    {
        pos->second++;
        filecount = pos->second;
    }
    else
    {
        filecount = 1;
        ms_filemap[cstrPath] = filecount;
    }
#endif
    char szTmp[cstrPath.size()+20];
//modify by luguanhuang 20180905
	int filecount = 1;
    if (filecount == 1)
    {
        snprintf(szTmp, sizeof(szTmp), "%s.log", cstrPath.c_str());
    }
    else
    {
        //snprintf(szTmp, sizeof(szTmp), "%s%d.log", cstrPath.c_str(), filecount);
        snprintf(szTmp, sizeof(szTmp), "%s.log", cstrPath.c_str());
    }

    // 产生并保存日志文件名 
    m_logfname = szTmp;

    // 提取日志文件的路径 
    char *ps = (char *)rindex(m_logfname.c_str(), '/');

    if (NULL == ps)
    {
        m_logfpath = "./";
    }
    else
    {
        m_logfpath.assign(m_logfname.c_str(), ps-m_logfname.c_str()+1);
    }

    m_flag = 0;
}

/*****************************************************************************
  *   用    途 : 获取当前日志级别的名称 
  *   参    数 : 
  *   返 回 值 : 日志级别的名称 
  *   文 件 名 : clog.cpp
  *   作    者 :  
  *   完成日期 :  
  *   描    述 :
  *   修改记录 :
  *   数据结构 : 
  *   备    注 :
*****************************************************************************/
const char* CLog::getLogLevelName(void)
{
    return getLogLevelName(getLogLevel());
}

/*****************************************************************************
  *   用    途 : 获取日志级别的名称 
  *   参    数 : 
  *     int lvl             当前的日志级别 
  *
  *   返 回 值 : 日志级别的名称 
  *   文 件 名 : clog.cpp
  *   作    者 :  
  *   完成日期 :  
  *   描    述 :
  *   修改记录 :
  *   数据结构 : 
  *   备    注 :
*****************************************************************************/
const char* CLog::getLogLevelName(int lvl)
{
    if ((unsigned int)lvl > CLOG_NONE)
    {
        return s_lvlname[CLOG_DEBUG];
    }

    return s_lvlname[(unsigned int)lvl];
}

/*****************************************************************************
  *   用    途 : 设置日志级别
  *   参    数 : 
  *     int lvl             日志级别 
  *
  *   返 回 值 : 新的日志级别
  *   文 件 名 : clog.cpp
  *   作    者 :  
  *   完成日期 :  
  *   描    述 :
  *   修改记录 :
  *   数据结构 : 
  *   备    注 :
*****************************************************************************/
int CLog::setLogLevel(int lvl)
{
    if ((unsigned int)lvl > CLOG_NONE)
    {
        return m_logLevel = CLOG_DEBUG;
    }

    return m_logLevel = (unsigned int)lvl;
}

/*****************************************************************************
  *   用    途 : 设置日志级别, 用字符串的方式设置 
  *   参    数 : 
  *     const char *szLvl   日志级别 
  *
  *   返 回 值 : 新的日志级别
  *   文 件 名 : clog.cpp
  *   作    者 :  
  *   完成日期 :  
  *   描    述 :
  *   修改记录 :
  *   数据结构 : 
  *   备    注 :
*****************************************************************************/
int CLog::setLogLevel(const char *szLvl)
{
    // 如果设置的日志级别不存在, 采用DEBUG的级别 
    for (int i=0; i<CLOG_NONE+1; i++)
    {
        if (!strcasecmp(szLvl, s_lvlname[i]))
        {
            setLogLevel(i);
            return 0;
        }
    }
    setLogLevel(CLOG_DEBUG);

    return -1;
}


/*****************************************************************************
  *   用    途 : 获取日志级别
  *   参    数 : 
  *   返 回 值 : 日志级别
  *   文 件 名 : clog.cpp
  *   作    者 :  
  *   完成日期 :  
  *   描    述 :
  *   修改记录 :
  *   数据结构 : 
  *   备    注 :
*****************************************************************************/
int CLog::getLogLevel(void)
{
    return m_logLevel;
}

/*****************************************************************************
  *   用    途 : 检测当前打开的日志文件的大小是否超限, 如果超限
  *              就自动转存 
  *   参    数 : 
  *   返 回 值 : 成功返回0, 否则返回错误码 
  *   文 件 名 : clog.cpp
  *   作    者 :  
  *   完成日期 :  
  *   描    述 :
  *   修改记录 :
  *   数据结构 : 
  *   备    注 :
*****************************************************************************/
int CLog::chkLogFileSize(void)
{
    if ( ftell(m_logfp) > m_maxFileSize )
    {
        char *ps;
        char sCmd[2*MAX_FILENAME_LEN];
        string str_bakfname;

        if ( NULL == (ps = (char *)rindex(m_logfname.c_str(), '/')) )
        {
            ps = (char*)m_logfname.c_str();
        }
        else
        {
            ps += 1;
        }

        memset(sCmd, 0x00, sizeof(sCmd));

        str_bakfname = m_logfpath + "bak/" + ps;
        if (!access("/bin/cp", X_OK))
        {
            if ( !access("/bin/gzip", X_OK) )
            {
                // 拷贝备份文件 
                snprintf(sCmd, sizeof(sCmd), 
                         "/bin/cp -f %s %s 1>/dev/null 2>&1\n",
                          m_logfname.c_str(), str_bakfname.c_str());
                system(sCmd);

                // 压缩备份文件 
                if ( !access(str_bakfname.c_str(), X_OK))
                {
                    snprintf(sCmd, sizeof(sCmd), 
                            "/bin/gzip -f %s\n", 
                            str_bakfname.c_str());
                    system(sCmd);
                }
            }
            else
            {
                // 拷贝备份文件 
                snprintf(sCmd, sizeof(sCmd), 
                         "cp -f %s %s 1>/dev/null 2>/dev/null",
                          m_logfname.c_str(), str_bakfname.c_str() );
                system(sCmd);
            }
        }

        if ( ftruncate(fileno(m_logfp), 0) != 0 )
        {
            return -1;
        }

        rewind(m_logfp);
    }

    return 0;
}


/*****************************************************************************
  *   用    途 : 打开日志文件 
  *   参    数 : 
  *   返 回 值 : 成功返回0, 否则返回错误码 
  *   文 件 名 : clog.cpp
  *   作    者 :  
  *   完成日期 :  
  *   描    述 :
  *   修改记录 :
  *   数据结构 : 
  *   备    注 :
*****************************************************************************/
int CLog::openLog(void)
{
    if (m_logfp)
    {
        fclose(m_logfp);
    }

    if (NULL == (m_logfp = fopen(m_logfname.c_str(), "a")))
    {
        if (errno == ENOENT )
        {
            return -1;
        }
        else if (errno == EPERM )
        {
            return -1;
        }

        return -1;
    }

    return 0;
}

/*****************************************************************************
  *   用    途 : 打开日志文件 
  *   参    数 : 
  *   返 回 值 : 成功返回0, 否则返回错误码 
  *   文 件 名 : clog.cpp
  *   作    者 :  
  *   完成日期 :  
  *   描    述 :
  *   修改记录 :
  *   数据结构 : 
  *   备    注 :
*****************************************************************************/
int CLog::closeLog(void)
{
    if (m_logfp)
    {
        fclose(m_logfp);
        m_logfp = NULL;
        return 0;
    }

    return 0;
}


/*****************************************************************************
  *   用    途 : 日志类调试日志写函数。
  *   参    数 : 
  *     int lvl             当前要写的日志级别 
  *     const char* file
  *     const char* func
  *     const int line
  *     const char* fmt
  *       ...
  *   返 回 值 : int 0表示成功，-1表示失败
  *   文 件 名 : clog.cpp
  *   作    者 :  
  *   完成日期 :  
  *   描    述 :
  *   修改记录 :
  *   数据结构 : 
  *   备    注 :
*****************************************************************************/
int  CLog::write(int lvl, const char *file, const char *func, int line, const char *fmt, ...)
//int  CLog::write(int lvl, const char *fmt, ...)
{
    int ret = 0;

    if ( (lvl == CLOG_NONE) || (lvl < m_logLevel) )
    {
        return 0;
    }


    if (!file || !func)
    {
        return -1;
    }


    pthread_mutex_lock(&m_mutex);
    if (0 != (ret = openLog()))
    {
        pthread_mutex_unlock(&m_mutex);
        return ret;
    }

	pthread_t thread_id = pthread_self();
    fprintf(m_logfp, "(t%d) [%s %s] [%s] [%s:%d] [%s]\n\t", 
                     thread_id, 
                     m_datetime.getDate(), m_datetime.getMiTime(), 
                     getLogLevelName(lvl),
                     file, line, func);

    va_list ap;
    va_start(ap, fmt);    
    vfprintf(m_logfp, fmt, ap);
    va_end(ap);
    
    fprintf(m_logfp, "\n");

    if (0 != (ret = chkLogFileSize()) )
    {
        closeLog();
        pthread_mutex_unlock(&m_mutex);
        return ret;
    }
    
    ret = closeLog();
    pthread_mutex_unlock(&m_mutex);

    return ret;
}


/*****************************************************************************
  *   用    途 : 更改当前日志的名称 
  *   参    数 : 新的日志文件名 
  *   返 回 值 : 0表示成功, 其他表示失败
  *   文 件 名 : clog.cpp
  *   作    者 :  
  *   完成日期 :  
  *   描    述 :
  *   修改记录 :
  *   数据结构 : 
  *   备    注 :
*****************************************************************************/
int  CLog::rename(const char *fname)
{
    if (!fname) 
    {
        return -1;
    }

    pthread_mutex_lock(&m_mutex);
    string strNewFile;
    strNewFile = string(fname) + ".log";

    char szTmp[2048] = {0};

    if (!access("/bin/cat", X_OK))
    {
//        snprintf(szTmp, sizeof(szTmp), "/bin/cat %s >> %s", m_logfname.c_str(), strNewFile.c_str());
        snprintf(szTmp, sizeof(szTmp), ">> %s", strNewFile.c_str());
        system(szTmp);
    }


    if (!access("/bin/rm", X_OK))
    {
        snprintf(szTmp, sizeof(szTmp), "rm -f %s", m_logfname.c_str());
        system(szTmp);
    }

    m_logfname = strNewFile;
    pthread_mutex_unlock(&m_mutex);

    return 0;
}

/*
 * vi: sw=4 ts=4 et
 */

