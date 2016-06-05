
/* author : limingfan
 * date : 2014.10.28
 * description : 提供各种日志输出、日志管理功能
 */
 
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "CLogger.h"
#include "MacroDefine.h"
#include "CCfg.h"


namespace NCommon
{

static const char* LogLvStr[] = {
	"INFO|",
	"WARN|",
	"ERROR|",
};

CLogger::CLogger(const char* pFullName, long size, int maxBakFile, int output)
{
	m_pDir = NULL;
	m_pName = NULL;
	m_dirSize = 0;
	m_maxSize = size;
	m_curSize = 0;
	m_maxBakFile = (maxBakFile > 0) ? maxBakFile : 1;
	m_idx = 1;
	m_fd = -1;
	m_output = output;
	
	m_writeErrTimes = 0;     // 连续写多少次失败后，重新关闭&打开文件
	m_tryTimes = 0;          // 连续尝试打开多少次文件失败后，关闭日志功能
	memset(m_fullName, 0, sizeof(m_fullName));
	
	setFileName(pFullName);
	setDirName(pFullName);
}

CLogger::~CLogger()
{
    closeFile();
	DELETE_ARRAY(m_pDir);
	DELETE_ARRAY(m_pName);
	m_dirSize = 0;
	m_maxSize = 0;
	m_curSize = 0;
	m_maxBakFile = 0;
	m_idx = 1;
	m_fd = -1;
	m_output = 0;
	m_writeErrTimes = 0;     // 连续写多少次失败后，重新关闭&打开文件
	m_tryTimes = 0;          // 连续尝试打开多少次文件失败后，关闭日志功能
	memset(m_fullName, 0, sizeof(m_fullName));
}

void CLogger::setFileName(const char* pFullName)
{
	// 文件名
	if (pFullName == NULL || *pFullName == '\0')  // 不存在文件名
	{
		return;
	}
		
	const char* pFind = strrchr(pFullName, '/');
	if (pFind != NULL)
	{
		pFullName = pFind + 1;
		if (*pFullName == '\0')  // 不存在文件名
		{
			return;
		}
	}
	
	int fileNameSize = strlen(pFullName);
	NEW_ARRAY(m_pName, char[fileNameSize + 1]);
	memcpy(m_pName, pFullName, fileNameSize);
	m_pName[fileNameSize] = '\0';
}

void CLogger::setDirName(const char* pFullName)
{
	if (pFullName == NULL || *pFullName == '\0')  // 不存在文件名
	{
		return;
	}
	
	int dirNameSize = 0;
	char* pDir = NULL;
	const int max = 50;  // 最多支持max级目录
	const char* dirArray[max];
	memset(dirArray, 0, sizeof(char*) * max);
	
	const char* pFind = strchr(pFullName, '/');
	while (pFind != NULL)
	{
		// 目录名处理
		dirNameSize = pFind - pFullName;
		NEW_ARRAY(pDir, char[dirNameSize + 1]);
		memcpy(pDir, pFullName, dirNameSize);
		pDir[dirNameSize] = '\0';
		dirArray[m_dirSize] = pDir;
		m_dirSize++;
		
		pFind = strchr(++pFind, '/');  // 继续查找下一级目录名
	}
	if (m_dirSize > 0)
	{
		NEW_ARRAY(m_pDir, const char*[m_dirSize]);
		for (int i = 0; i < m_dirSize; i++)
		{
			m_pDir[i] = dirArray[i];
		}
	}
}

bool CLogger::checkCurrentFileIsExist()
{
	return (access(m_fullName, F_OK | W_OK) == 0);
}

int CLogger::openCurrentFile()
{	
	if (createDir())
	{
		// 找到最近可以写的日志文件
		const char* pPath = (m_dirSize > 0) ? m_pDir[m_dirSize - 1] : "";
		int curIdx = 1;
		int fLen = 0;
		for (int i = 1; i <= m_maxBakFile; i++)
		{
			fLen = snprintf(m_fullName, MaxFullLen - 1, "%s/%s%d.log", pPath, m_pName, i);
			m_fullName[fLen] = '\0';
			if (access(m_fullName, F_OK | W_OK) == 0)  // 存在该文件了
			{
				curIdx = i;
			}
		}
		
		curIdx = (curIdx >= m_maxBakFile) ? 1 : curIdx;
		fLen = snprintf(m_fullName, MaxFullLen - 1, "%s/%s%d.log", pPath, m_pName, curIdx);
		m_fullName[fLen] = '\0';
		struct stat fileInfo;
		m_curSize = 0;
		if (stat(m_fullName, &fileInfo) == 0)  // 如果文件存在则取文件长度
		{
			m_curSize = fileInfo.st_size;
		}
		
		// 打开文件，不存在则创建
		m_fd = open(m_fullName, O_WRONLY | O_APPEND | O_CREAT , S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		if (m_fd >= 0)
		{
			m_idx = curIdx;
		}
	}
	
	if (m_fd >= 0)
	{
		m_tryTimes = 0;
	}
	else if (m_tryTimes <= TryTimes)
	{
		m_tryTimes++;
		m_fullName[0] = '\0';
	}
	
	return m_fd;
}

int CLogger::openNextFile(int idx, char* fullName, unsigned int len)
{
	int fd = -1;
	if (createDir())
	{
		const char* pPath = (m_dirSize > 0) ? m_pDir[m_dirSize - 1] : "";
		int fLen = snprintf(fullName, len - 1, "%s/%s%d.log", pPath, m_pName, idx);
		fullName[fLen] = '\0';

		// 打开文件，不存在则创建，存在则清空之前的内容
		fd = open(fullName, O_WRONLY | O_APPEND | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	}
	
	if (fd >= 0)
	{
		m_tryTimes = 0;
	}
	else if (m_tryTimes <= TryTimes)
	{
		m_tryTimes++;
	}
	
	return fd;
}

void CLogger::checkAndNewFile(int wLen)
{
	m_curSize += wLen;
	if (m_curSize >= m_maxSize)  // 超出文件最大长度了
	{
		char fullName[MaxFullLen] = {0};   // 当前文件名
		int curIdx = (m_idx >= m_maxBakFile) ? 1 : (m_idx + 1);	
		int fd = openNextFile(curIdx, fullName, MaxFullLen);  // 创建下一个日志文件
		if (fd >= 0)
		{
			closeFile();  // 关闭之前的文件
			m_fd = fd;
			m_idx = curIdx;
			m_curSize = 0;
			strcpy(m_fullName, fullName);
		}
	}
}

void CLogger::closeFile()
{
	if (m_fd >= 0)
	{
		close(m_fd);
		m_fd = -1;
	}
	m_fullName[0] = '\0';
}

void CLogger::setOutput(int v)
{
	m_output = v;
}

int CLogger::writeFile(const char* fileName, const int fileLine, const LogLevel logLv, const char* pFormat, ...)
{
	if (m_output == 0)
	{
		closeFile();
		return 0;
	}
	
	if (m_fd < 0 || !checkCurrentFileIsExist())
	{
		closeFile();
		openCurrentFile();
	}
	
	int wLen = 0;
	if (m_fd >= 0)
	{
		// 增加日志的日期时间
		static char logBuff[MaxLogBuffLen] = {'\0'};  // 最大支持一次写入的日志长度
		struct timeval tv;
		gettimeofday(&tv, NULL);
		struct tm* pTm = localtime(&tv.tv_sec);
		wLen = formatHead(logBuff, MaxLogBuffLen - 1, pTm, tv, logLv, fileName, fileLine);
		if (wLen > 0)
		{
			va_list valp;
			va_start(valp, pFormat);
			int logN = vsnprintf(logBuff + wLen, MaxLogBuffLen - wLen - 1, pFormat, valp);
			va_end(valp);
			if (logN > 0)
			{
				wLen += logN;
			}
			
			if (wLen >= MaxLogBuffLen)
			{
				wLen = MaxLogBuffLen - 1;
			}
			logBuff[wLen] = '\n';  // 增加换行符
			wLen = write(m_fd, logBuff, wLen + 1);
			if (wLen > 0)
			{
				checkAndNewFile(wLen);
			}
		}
		
		reOpenFile(wLen);  // 错误检查
	}
	return wLen;
}

int CLogger::formatHead(char* logBuff, const int len, const struct tm* pTm, const struct timeval& tv, const LogLevel logLv,
				        const char* fileName, const int fileLine)
{
	if (fileName != NULL)
	{
		const char* onlyName = strrchr(fileName, '/');
		if (onlyName != NULL)
		{
			fileName = ++onlyName;
		}
	}
	
	const char* lvStr = LogLvStr[logLv];
	int wLen = 0; 
	if (pTm != NULL)
	{
		if (fileName == NULL)
		{
			wLen = snprintf(logBuff, len, "%d-%02d-%02d %02d:%02d:%02d.%03d|%s", (pTm->tm_year + 1900), (pTm->tm_mon + 1), pTm->tm_mday,
		    pTm->tm_hour, pTm->tm_min, pTm->tm_sec, (int)(tv.tv_usec / 1000), lvStr);
		}
		else
		{
		    wLen = snprintf(logBuff, len, "%d-%02d-%02d %02d:%02d:%02d.%03d|%s:%d %s", (pTm->tm_year + 1900), (pTm->tm_mon + 1), pTm->tm_mday,
		    pTm->tm_hour, pTm->tm_min, pTm->tm_sec, (int)(tv.tv_usec / 1000), fileName, fileLine, lvStr);
		}
	}
	else
	{
		wLen = snprintf(logBuff, len, "0000-00-00 00:00:00.000|%s:%d %s", (fileName == NULL) ? "" : fileName, fileLine, lvStr);
	}
	
	return wLen;
}

void CLogger::reOpenFile(int wLen)
{
	if (wLen >= 0)
	{
		m_writeErrTimes = 0;
	}
	else
	{
		m_writeErrTimes++;
		if (m_writeErrTimes >= WriteErrTime)
		{
			m_writeErrTimes = 0;
			closeFile();
			openCurrentFile();
		}
	}
}

bool CLogger::createDir()
{
	if (m_pName == NULL || *m_pName == '\0' || m_tryTimes >= TryTimes)
	{
		return false;
	}
	
	// 检查目录是否都在，不在则创建
	bool checkDirOk = true;
	for (int i = 0; i < m_dirSize; i++)
	{
		if (*(m_pDir[i]) != '\0')
		{
		    int rc = access(m_pDir[i], F_OK | W_OK);
			if (rc != 0)  // 不存在该目录则创建
			{
				rc = mkdir(m_pDir[i], S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);  // 保持和手动创建的目录权限一致
				if (rc != 0)  // 创建目录失败直接退出
				{
					checkDirOk = false;
					break;
				}
			}
		}
	}
	
	return checkDirOk;
}



static const char* loggerSection = "Logger";
struct LoggerConfigChecker
{
	LoggerConfigChecker()
	{
		const char* cfgItem[] = {"DebugLogFile", "ReleaseLogFile", "OptLogFile", "WriteDebugLog", "LogFileSize", "LogBakFileCount"};
		for (unsigned int i = 0; i < (sizeof(cfgItem) / sizeof(cfgItem[0])); ++i)
		{
			const char* value = CCfg::getValue(loggerSection, cfgItem[i]);
			if (value == NULL)
			{
				printf("the logger config is error, [%s] : %s value can not be null!\n", loggerSection, cfgItem[i]);
			    exit(1);
			}
			
			if (i > 3 && atoi(value) < 1)
			{
				printf("the logger config is error, [%s] : %s value can not less than 1!\n", loggerSection, cfgItem[i]);
				exit(1);
			}
		}
	}
};

// 日志文件的相关配置数据
// 全局唯一日志对象
CLogger& CLogger::getDebugLogger()
{
	static const LoggerConfigChecker loggerConfigChecker;
	static CLogger debugLog(CCfg::getValue(loggerSection, "DebugLogFile"),
	                        atoi(CCfg::getValue(loggerSection, "LogFileSize")),
							atoi(CCfg::getValue(loggerSection, "LogBakFileCount")),
							atoi(CCfg::getValue(loggerSection, "WriteDebugLog")));
	return debugLog;
}

CLogger& CLogger::getReleaseLogger()
{
	static const LoggerConfigChecker loggerConfigChecker;
	static CLogger releaseLog(CCfg::getValue(loggerSection, "ReleaseLogFile"),
							  atoi(CCfg::getValue(loggerSection, "LogFileSize")),
							  atoi(CCfg::getValue(loggerSection, "LogBakFileCount")));
	return releaseLog;
}

CLogger& CLogger::getOptLogger()
{
	static const LoggerConfigChecker loggerConfigChecker;
	static CLogger optLog(CCfg::getValue(loggerSection, "OptLogFile"),
						  atoi(CCfg::getValue(loggerSection, "LogFileSize")),
						  atoi(CCfg::getValue(loggerSection, "LogBakFileCount")));
	return optLog;
}
	


// 以下为测试代码
// only for test code
void CLogger::output()
{
	const char* name = m_pName ? m_pName : "no input file name";
	printf("File Name = %s\n", name);
	for (int i = 0; i < m_dirSize; i++)
	{
		printf("Dir Name = %s\n", m_pDir[i]);
	}
}

void CLogger::testWriteFile(int n)
{
	const char* pLogs[] = {
		"11111sdfsdfd********8888sfdsfdsfafsd1A",
		"2222223435dfvdfJJJJJJJJJJJJJJJJJJJJJJgfrgfdgds!!!$%^&22B",
		"3334456565mm<>?TYHxxxxxxxxxxxxxxxGHSDFGHJTY333C"
	};
	for (int t = 0; t < n; t++)
	{
		for (unsigned int i = 0; i < (sizeof(pLogs) / sizeof(pLogs[0])); i++)
		{
			writeFile(NULL, 0, Info, "%s---%d---%c", pLogs[i], i + 1, pLogs[i][strlen(pLogs[i]) - 1]);
		}
	}
}


}

