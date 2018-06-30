
/* author : limingfan
 * date : 2015.02.27
 * description : MySql数据库操作API封装
 */

#include <string.h>
#include <stdio.h>
#include <string>

#include "CMySql.h"


using namespace NCommon;
using namespace NErrorCode;

namespace NDBOpt
{

// 结果集状态
enum ResultStatus
{
	singleResult = 0,     // 单一结果集
	multiResult = 1,      // 多语句查询第一个结果集
	nextToResult = 2,     // 多语句查询下一个结果集
};

// 查询结果集合数据
// 查询结果集合数据
CQueryResult::CQueryResult() : m_dbOpt(NULL), m_result(NULL), m_resultStatus(singleResult)
{
}

CQueryResult::CQueryResult(CDBOpertaion* dbOpt, MYSQL_RES* result, int status) : m_dbOpt(dbOpt), m_result(result), m_resultStatus(status)
{
}

CQueryResult::~CQueryResult()
{
	reset();
}

void CQueryResult::reset(CDBOpertaion* dbOpt, MYSQL_RES* result, int status)
{
	if (m_dbOpt != NULL)
	{
		while (nextResult());  // 清空所有可能还存在的结果集，防止后面执行sql命令出错
		freeResult();
	}
	
	m_dbOpt = dbOpt;
	m_result = result;
	m_resultStatus = status;
}

RowDataPtr CQueryResult::getNextRow()
{
	return (m_result != NULL) ? mysql_fetch_row(m_result) : NULL;
}

unsigned long CQueryResult::getRowCount()
{
	return (m_result != NULL) ? mysql_num_rows(m_result) : 0;
}

FieldDataPtr CQueryResult::getNextField()
{
	return (m_result != NULL) ? mysql_fetch_field(m_result) : NULL;
}

FieldDataPtr CQueryResult::getAllFields()
{
	return (m_result != NULL) ? mysql_fetch_fields(m_result) : NULL;
}

FieldDataPtr CQueryResult::getField(unsigned int idx)
{
	return (m_result != NULL) ? mysql_fetch_field_direct(m_result, idx) : NULL;
}

unsigned int CQueryResult::getFieldCount()
{
	return (m_result != NULL) ? mysql_num_fields(m_result) : 0;
}

unsigned long* CQueryResult::getFieldLengthsOfCurrentRow()
{
	return (m_result != NULL) ? mysql_fetch_lengths(m_result) : NULL;
}

// 是否存在下一个结果集，多语句执行或者存储过程执行有可能会返回多个结果集
bool CQueryResult::nextResult()
{
	if (m_resultStatus == singleResult) return false;  // 单一结果集
	
	if (m_resultStatus == multiResult)
	{
		m_resultStatus = nextToResult;      // 多语句执行结果集
		if (m_result != NULL) return true;  // 第一个执行语句已经提前获取结果集了，如果存在则直接返回
	}
	
	if (m_dbOpt == NULL) return false;
	
	freeResult();
	MYSQL* handler = m_dbOpt->getHandler();
	int status = -1;
    do
	{
	    status = mysql_next_result(handler);
		if (status == -1) return false;     // all finished
		if (status > 0)                     // error
		{
			ReleaseErrorLog("get next result status error = %d, status = %s, info = %s.", m_dbOpt->errorNum(), m_dbOpt->stateInfo(), m_dbOpt->errorInfo());
			return false;
		}

		m_result =  mysql_store_result(handler);
		if (m_result != NULL) return true;  // ok, get next result
		
		if (mysql_field_count(handler) != 0)
		{
			ReleaseErrorLog("get more result error = %d, status = %s, info = %s.", m_dbOpt->errorNum(), m_dbOpt->stateInfo(), m_dbOpt->errorInfo());
			return false;
		}
		
	} while (true);
	
	return false;  // never come here
}

void CQueryResult::freeResult()
{
	if (m_result != NULL)
	{
		mysql_free_result(m_result);
	    m_result =  NULL;
	}
}




// 预处理执行器，加快执行性能
CPreparedStmt::CPreparedStmt(CDBOpertaion* dbOpt, MYSQL_STMT* stmt) : m_dbOpt(dbOpt), m_stmt(stmt), m_resultMode(0)
{
	
}

CPreparedStmt::~CPreparedStmt()
{
	mysql_stmt_close(m_stmt);
	m_stmt = NULL;
	m_dbOpt = NULL;
}

// 操作错误信息
unsigned int CPreparedStmt::errorNum()
{
	return mysql_stmt_errno(m_stmt);
}

const char* CPreparedStmt::errorInfo()
{
	return mysql_stmt_error(m_stmt);
}

const char* CPreparedStmt::stateInfo()
{
	return mysql_stmt_sqlstate(m_stmt);
}


// 执行sql命令操作
// mode参数 0：一次取回所有查询的结果集数据；非0：结果集分执行动作每次取回一个； 默认值为0
void CPreparedStmt::setResultMode(char mode)
{
	m_resultMode = mode;
}
 
int CPreparedStmt::prepare(const char* stmtStr, unsigned long length)
{
	if (stmtStr == NULL || length == 0) return InvalidParam;
	int rc = mysql_stmt_prepare(m_stmt, stmtStr, length);
	if (rc != 0)
	{
		ReleaseErrorLog("prepare stmt error = %d, status = %s, info = %s.", errorNum(), stateInfo(), errorInfo());
		return DoPreStmtCmmError;
	}
	
	return Success;
}

bool CPreparedStmt::bindParam(BindData* bind)
{
	return (mysql_stmt_bind_param(m_stmt, bind) == 0);
}

bool CPreparedStmt::bindResult(BindData* bind)
{
	return (mysql_stmt_bind_result(m_stmt, bind) == 0);
}

int CPreparedStmt::execute()
{
	if (mysql_stmt_execute(m_stmt) != 0)
	{
		ReleaseErrorLog("execute stmt error = %d, status = %s, info = %s.", errorNum(), stateInfo(), errorInfo());
		return DoPreStmtCmmError;
	}
	
	if (m_resultMode == 0 && mysql_stmt_store_result(m_stmt) != 0)
	{
		ReleaseWarnLog("get all stmt execute result error = %d, status = %s, info = %s.", errorNum(), stateInfo(), errorInfo());
	}
	
	return Success;
}

bool CPreparedStmt::reset()
{
	return (mysql_stmt_reset(m_stmt) == 0);
}

// 存储过程调用，存在多个结果集
bool CPreparedStmt::nextResult()
{
	int rc = mysql_stmt_next_result(m_stmt);
	if (rc == 0) return true;
	if (rc > 0)
	{
		ReleaseWarnLog("get next result data error = %d, status = %s, info = %s.", errorNum(), stateInfo(), errorInfo());
	}
	
	return false;
}

// 释放prepare结果集资源
bool CPreparedStmt::freeResult()
{
	return (mysql_stmt_free_result(m_stmt) == 0);
}


// 结果集行数据
bool CPreparedStmt::toNextRow()
{
	int rc = mysql_stmt_fetch(m_stmt);
	if (rc == 0) return true;
	if (rc != MYSQL_NO_DATA)
	{
		ReleaseWarnLog("get next row data error = %d, status = %s, info = %s.", errorNum(), stateInfo(), errorInfo());
	}
	
	return false;
}

void CPreparedStmt::seekToRow(const unsigned long long row)
{
	mysql_stmt_data_seek(m_stmt, row);
}

unsigned long CPreparedStmt::getAffectedRows()
{
	return (unsigned long)mysql_stmt_affected_rows(m_stmt);
}

unsigned long CPreparedStmt::getRowCount()
{
	return (unsigned long)mysql_stmt_num_rows(m_stmt);
}

unsigned int CPreparedStmt::getFieldCount()
{
	return mysql_stmt_field_count(m_stmt);
}

unsigned long CPreparedStmt::getParamCount()
{
	return mysql_stmt_param_count(m_stmt);
}
	
	


// 单个数据库操作，每个实例对应操作一个数据库
CDBOpertaion::CDBOpertaion(const char* host, const char* user, const char* db)
{
	strncpy(m_host, host, MaxNameLen - 1);
	strncpy(m_user, user, MaxNameLen - 1);
	strncpy(m_dbName, db, MaxNameLen - 1);
}

CDBOpertaion::~CDBOpertaion()
{
	m_host[0] = '\0';
	m_user[0] = '\0';
	m_dbName[0] = '\0';
}


// 操作错误信息
unsigned int CDBOpertaion::errorNum()
{
	return mysql_errno(&m_mysqlHandler);
}

const char* CDBOpertaion::errorInfo()
{
	return mysql_error(&m_mysqlHandler);
}

const char* CDBOpertaion::stateInfo()
{
	return mysql_sqlstate(&m_mysqlHandler);
}


// 执行任意合法的sql语句
int CDBOpertaion::executeSQL(const char* sql)
{
	if (sql == NULL || *sql == '\0') return InvalidParam;
	return executeSQL(sql, strlen(sql));
}

int CDBOpertaion::executeSQL(const char* sql, const unsigned int len)
{
	if (sql == NULL || len == 0) return InvalidParam;

	if(mysql_real_query(&m_mysqlHandler, sql, len) != 0)
	{
		const unsigned int LostConnectionError = 2013;
		if (LostConnectionError == errorNum())
		{
			if (mysql_real_query(&m_mysqlHandler, sql, len) != 0)  // 断连则尝试重连
			{
				std::string strSql(sql, len);
				ReleaseErrorLog("redo execute sql error = %d, status = %s, info = %s, sql len = %d, data = %s",
				errorNum(), stateInfo(), errorInfo(), len, strSql.c_str());
				return DoSqlCmmError;
			}
		}
		else
		{
			std::string strSql(sql, len);
			ReleaseErrorLog("execute sql error = %d, status = %s, info = %s, sql len = %d, data = %s",
			errorNum(), stateInfo(), errorInfo(), len, strSql.c_str());
			return DoSqlCmmError;
		}
	}
	
    return Success;
}


// 创建&删除 数据库表
int CDBOpertaion::createTable(const char* sql)
{
	return executeSQL(sql);
}

int CDBOpertaion::createTable(const char* sql, const unsigned int len)
{
	return executeSQL(sql, len);
}

int CDBOpertaion::dropTable(const char* tableName)
{
	if (tableName == NULL || *tableName == '\0') return InvalidParam;
	
	char dropSql[MaxNameLen] = {0};
	const unsigned int len = snprintf(dropSql, MaxNameLen - 1, "DROP TABLE IF EXISTS %s;", tableName);
	return executeSQL(dropSql, len);
}


// 修改数据库表数据
int CDBOpertaion::modifyTable(const char* sql)
{
	return executeSQL(sql);
}

int CDBOpertaion::modifyTable(const char* sql, const unsigned int len)
{
	return executeSQL(sql, len);
}

unsigned long CDBOpertaion::getAffectedRows()
{
	return (unsigned long)mysql_affected_rows(&m_mysqlHandler);
}


// 查询数据库表，一次性获取全部查询结果集合
int CDBOpertaion::queryTableAllResult(const char* sql, CQueryResult& qResult)
{
	if (sql == NULL || *sql == '\0') return InvalidParam;
	return queryTableAllResult(sql, strlen(sql), qResult);
}

int CDBOpertaion::queryTableAllResult(const char* sql, const unsigned int len, CQueryResult& qResult)
{
	qResult.reset();  // 先重置，释放可能存在的结果集

    MYSQL_RES* result = NULL;
	int rc = queryTableAllResult(sql, len, result);
	if (rc != Success) return rc;
	
	if (result != NULL) qResult.reset(this, result, singleResult);

	return Success;
}

int CDBOpertaion::queryTableAllResult(const char* sql, CQueryResult*& qResult)
{
	if (sql == NULL || *sql == '\0') return InvalidParam;
	return queryTableAllResult(sql, strlen(sql), qResult);
}

int CDBOpertaion::queryTableAllResult(const char* sql, const unsigned int len, CQueryResult*& qResult)
{
	MYSQL_RES* result = NULL;
	int rc = queryTableAllResult(sql, len, result);
	if (rc != Success) return rc;
	
	if (result != NULL)
	{
		NEW(qResult, CQueryResult(this, result, singleResult));
	    if (qResult == NULL) return NoMemory;
	}
	else
	{
		qResult = NULL;  // 当前查询没有结果集
	}
	
	return Success;
}

// 查询数据库表，需要依次获取查询结果集合
int CDBOpertaion::queryTableResult(const char* sql, CQueryResult& qResult)
{
	if (sql == NULL || *sql == '\0') return InvalidParam;
	return queryTableResult(sql, strlen(sql), qResult);
}

int CDBOpertaion::queryTableResult(const char* sql, const unsigned int len, CQueryResult& qResult)
{
	qResult.reset();  // 先重置，释放可能存在的结果集

    MYSQL_RES* result = NULL;
	int rc = queryTableResult(sql, len, result);
	if (rc != Success) return rc;
	
	if (result != NULL) qResult.reset(this, result, singleResult);

	return Success;
}

int CDBOpertaion::queryTableResult(const char* sql, CQueryResult*& qResult)
{
	if (sql == NULL || *sql == '\0') return InvalidParam;
	return queryTableResult(sql, strlen(sql), qResult);
}

int CDBOpertaion::queryTableResult(const char* sql, const unsigned int len, CQueryResult*& qResult)
{
	MYSQL_RES* result = NULL;
	int rc = queryTableResult(sql, len, result);
	if (rc != Success) return rc;
	
	if (result != NULL)
	{
		NEW(qResult, CQueryResult(this, result, singleResult));
	    if (qResult == NULL) return NoMemory;
	}
	else
	{
		qResult = NULL;  // 当前查询没有结果集
	}
	
	return Success;
}

// 使用完查询结果集对象后，一定记得释放该对象，否则内存泄露
void CDBOpertaion::releaseQueryResult(CQueryResult*& qResult)
{
	if (qResult != NULL) DELETE(qResult);
}


// 一次执行多条语句
int CDBOpertaion::executeMultiSql(const char* sql, CQueryResult& qResult)
{
	if (sql == NULL || *sql == '\0') return InvalidParam;
	return executeMultiSql(sql, strlen(sql), qResult);
}

int CDBOpertaion::executeMultiSql(const char* sql, const unsigned int len, CQueryResult& qResult)
{
	int rc = queryTableAllResult(sql, len, qResult);
	if (rc == Success)
	{
		qResult.m_dbOpt = this;
		qResult.m_resultStatus = multiResult;  // 多语句执行，第一条命令可能无结果集，但并不表示后面的语句也无结果集
    }

	return rc;
}

int CDBOpertaion::executeMultiSql(const char* sql, CQueryResult*& qResult)
{
	if (sql == NULL || *sql == '\0') return InvalidParam;
	return executeMultiSql(sql, strlen(sql), qResult);
}

int CDBOpertaion::executeMultiSql(const char* sql, const unsigned int len, CQueryResult*& qResult)
{
	int rc = queryTableAllResult(sql, len, qResult);
	if (rc == Success)
    {
        if (qResult == NULL) NEW(qResult, CQueryResult(this, NULL, multiResult));  // 多语句执行，第一条命令无结果集并不表示后面的语句也无结果集
		qResult->m_resultStatus = multiResult;
	}
	return rc;
}


// 预处理执行器
int CDBOpertaion::createPreparedStmt(CPreparedStmt*& preStmt)
{
	MYSQL_STMT* stmt = mysql_stmt_init(&m_mysqlHandler);
	if (stmt == NULL) return NoMemory;
	
	NEW(preStmt, CPreparedStmt(this, stmt));
	if (preStmt == NULL) return NoMemory;
	
	return Success;
}

void CDBOpertaion::releasePreparedStmt(CPreparedStmt*& preStmt)
{
	if (preStmt != NULL) DELETE(preStmt);
}


// 事务处理相关，一般情况下执行事务都使用sql语句执行实现而不调用API
bool CDBOpertaion::autoCommit(char mode)
{
	return (mysql_autocommit(&m_mysqlHandler, mode) == 0);
}

bool CDBOpertaion::commit()
{
	return (mysql_commit(&m_mysqlHandler) == 0);
}

bool CDBOpertaion::rollback()
{
	return (mysql_rollback(&m_mysqlHandler) == 0);
}

// 创建可在SQL语句中使用的合法SQL字符串
bool CDBOpertaion::realEscapeString(char* to, const char* from, unsigned long& length)
{
	const unsigned long retLength = mysql_real_escape_string(&m_mysqlHandler, to, from, length);
	if (retLength == (unsigned long)-1) return false;
	
	length = retLength;
	
	return true;
}


const char* CDBOpertaion::getHost()
{
	return m_host;
}

const char* CDBOpertaion::getUser()
{
	return m_user;
}

const char* CDBOpertaion::getDBName()
{
	return m_dbName;
}

MYSQL* CDBOpertaion::getHandler()
{
	return &m_mysqlHandler;
}


// 查询数据库表，一次性获取全部查询结果集合
int CDBOpertaion::queryTableAllResult(const char* sql, const unsigned int len, MYSQL_RES*& result)
{
	int rc = executeSQL(sql, len);
	if (rc != Success) return rc;
	
	result = mysql_store_result(&m_mysqlHandler);
	if (result == NULL)
	{
		if (mysql_field_count(&m_mysqlHandler) != 0)
		{
			ReleaseErrorLog("get all sql result error = %d, status = %s, info = %s.", errorNum(), stateInfo(), errorInfo());
			return GetSqlAllResultError;
		}
		
		// result == NULL 当前查询没有结果集
	}

	return Success;
}

// 查询数据库表，需要依次获取查询结果集合
int CDBOpertaion::queryTableResult(const char* sql, const unsigned int len, MYSQL_RES*& result)
{
	int rc = executeSQL(sql, len);
	if (rc != Success) return rc;
	
	result =  mysql_use_result(&m_mysqlHandler);
	if (result == NULL)
	{
		if (errorNum() != 0)
		{
			ReleaseErrorLog("get single sql result error = %d, status = %s, info = %s.", errorNum(), stateInfo(), errorInfo());
			return GetSqlResultError;
		}
		
		// result == NULL 当前查询没有结果集
	}
	
	return Success;
}


// MySql相关操作
int CMySql::createDBOpt(CDBOpertaion*& dbOpt, const char* host, const char* user, const char* passwd, const char* db, unsigned int port)
{
	if (!instance().m_isInitOk) return InitMySqlLibError;
	if (host == NULL || *host == '\0' || user == NULL || *user == '\0' || db == NULL || *db == '\0') return InvalidParam;

	NEW(dbOpt, CDBOpertaion(host, user, db));
	if (dbOpt == NULL) return NoMemory;
	
	MYSQL* handler = dbOpt->getHandler();
	::mysql_init(handler);
	int ret = 0;
	do
	{
		// 使用utf8字符集
		ret = ::mysql_options(handler, MYSQL_SET_CHARSET_NAME, "utf8");
		if(ret != 0)
		{
			ReleaseErrorLog("set charset name error = %d, status = %s, info = %s.", ::mysql_errno(handler), ::mysql_sqlstate(handler), ::mysql_error(handler));
			break;
		}

        // 默认设置，断连则自动重连接
		my_bool value = 1;
		ret = ::mysql_options(handler, MYSQL_OPT_RECONNECT, &value);
		if(ret != 0)
		{
			ReleaseErrorLog("set reconnect error = %d, status = %s, info = %s.", ::mysql_errno(handler), ::mysql_sqlstate(handler), ::mysql_error(handler));
			break;
		}
		
		// 默认设置，支持同时执行多语句
		if (::mysql_real_connect(handler, host, user, passwd, db, port, NULL, CLIENT_MULTI_STATEMENTS) == NULL)
		{
			ret = -1;
			ReleaseErrorLog("connect mysql server error = %d, status = %s, info = %s.", ::mysql_errno(handler), ::mysql_sqlstate(handler), ::mysql_error(handler));
			break;
		}
		
	} while (0);
	
	if (ret != 0)
	{
		destroyDBOpt(dbOpt);
		return ConnectMySqlError;
	}
	
	return Success;
}

void CMySql::destroyDBOpt(CDBOpertaion*& dbOpt)
{
	if (dbOpt != NULL)
	{
		::mysql_close(dbOpt->getHandler());
		DELETE(dbOpt);
	}
}

CMySql& CMySql::instance()
{
	static CMySql mysqlInstance;
	return mysqlInstance;
}

CMySql::CMySql()
{
	m_isInitOk = (::mysql_library_init(0, NULL, NULL) == 0);
}

CMySql::~CMySql()
{
	::mysql_library_end();
}


}

