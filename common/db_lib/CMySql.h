
/* author : limingfan
 * date : 2015.02.27
 * description : MySql数据库操作API封装
 */


/* MySql 预处理API开发绑定类型参考信息
MYSQL_BIND
This structure is used both for statement input (data values sent to the server) and output (result values returned from the server):
For input, use MYSQL_BIND structures with mysql_stmt_bind_param() to bind parameter data values to buffers for use by mysql_stmt_execute().
For output, use MYSQL_BIND structures with mysql_stmt_bind_result() to bind buffers to result set columns, for use in fetching rows with mysql_stmt_fetch().
To use a MYSQL_BIND structure, zero its contents to initialize it, then set its members appropriately. 
For example, to declare and initialize an array of three MYSQL_BIND structures, use this code:

MYSQL_BIND bind[3];
memset(bind, 0, sizeof(bind));
The MYSQL_BIND structure contains the following members for use by application programs. 
For several of the members, the manner of use depends on whether the structure is used for input or output.

enum enum_field_types buffer_type
This member indicates the data type of the C language variable bound to a statement parameter or result set column. 
For input, buffer_type indicates the type of the variable containing the value to be sent to the server. 
For output, it indicates the type of the variable into which a value received from the server should be stored. 

void *buffer
A pointer to the buffer to be used for data transfer. This is the address of a C language variable.
For input, buffer is a pointer to the variable in which you store the data value for a statement parameter. 
For output, buffer is a pointer to the variable in which to return a result set column value. 

unsigned long buffer_length
The actual size of *buffer in bytes. 
This indicates the maximum amount of data that can be stored in the buffer. 

unsigned long *length
A pointer to an unsigned long variable that indicates the actual number of bytes of data stored in *buffer. length is used for character or binary C data.

my_bool *is_null
This member points to a my_bool variable that is true if a value is NULL, false if it is not NULL. 
For input, set *is_null to true to indicate that you are passing a NULL value as a statement parameter.
For output, when you fetch a row, MySQL sets the value pointed to by is_null to true or false according to whether the result set column value returned from the statement is or is not NULL.

my_bool is_unsigned
This member applies for C variables with data types that can be unsigned (char, short int, int, long long int). 

my_bool *error
For output, set this member to point to a my_bool variable to have truncation information for the parameter stored there after a row fetching operation. 

 
MYSQL_TIME
This structure is used to send and receive DATE, TIME, DATETIME, and TIMESTAMP data directly to and from the server. 
The MYSQL_TIME structure contains the members listed in the following table.

Member	Description
unsigned int year	The year
unsigned int month	The month of the year
unsigned int day	The day of the month
unsigned int hour	The hour of the day
unsigned int minute	The minute of the hour
unsigned int second	The second of the minute
my_bool neg	A boolean flag indicating whether the time is negative
unsigned long second_part	The fractional part of the second in microseconds
Only those parts of a MYSQL_TIME structure that apply to a given type of temporal value are used. 
The year, month, and day elements are used for DATE, DATETIME, and TIMESTAMP values. 
The hour, minute, and second elements are used for TIME, DATETIME, and TIMESTAMP values.


MySql 预处理API开发绑定类型参考信息
// MySql 预处理绑定参数输入类型
Input Variable C Type	buffer_type Value	SQL Type of Destination Value
signed char	            MYSQL_TYPE_TINY	         TINYINT
short int	            MYSQL_TYPE_SHORT	     SMALLINT
int	                    MYSQL_TYPE_LONG	         INT
long long int	        MYSQL_TYPE_LONGLONG	     BIGINT
float	                MYSQL_TYPE_FLOAT	     FLOAT
double	                MYSQL_TYPE_DOUBLE	     DOUBLE
MYSQL_TIME	            MYSQL_TYPE_TIME	         TIME
MYSQL_TIME	            MYSQL_TYPE_DATE	         DATE
MYSQL_TIME	            MYSQL_TYPE_DATETIME	     DATETIME
MYSQL_TIME	            MYSQL_TYPE_TIMESTAMP	 TIMESTAMP
char[]	                MYSQL_TYPE_STRING	     TEXT, CHAR, VARCHAR
char[]	                MYSQL_TYPE_BLOB	         BLOB, BINARY, VARBINARY
 	                    MYSQL_TYPE_NULL	         NULL

// MySql 预处理绑定输出结果类型
SQL Type of Received Value	buffer_type Value	          Output Variable C Type
TINYINT	                    MYSQL_TYPE_TINY	                  signed char
SMALLINT	                MYSQL_TYPE_SHORT	              short int
MEDIUMINT	                MYSQL_TYPE_INT24	              int
INT	                        MYSQL_TYPE_LONG	                  int
BIGINT	                    MYSQL_TYPE_LONGLONG	              long long int
FLOAT	                    MYSQL_TYPE_FLOAT	              float
DOUBLE	                    MYSQL_TYPE_DOUBLE	              double
DECIMAL	                    MYSQL_TYPE_NEWDECIMAL	          char[]
YEAR	                    MYSQL_TYPE_SHORT	              short int
TIME	                    MYSQL_TYPE_TIME	                  MYSQL_TIME
DATE	                    MYSQL_TYPE_DATE	                  MYSQL_TIME
DATETIME	                MYSQL_TYPE_DATETIME	              MYSQL_TIME
TIMESTAMP	                MYSQL_TYPE_TIMESTAMP	          MYSQL_TIME
CHAR, BINARY	            MYSQL_TYPE_STRING	              char[]
VARCHAR, VARBINARY	        MYSQL_TYPE_VAR_STRING	          char[]
TINYBLOB, TINYTEXT	        MYSQL_TYPE_TINY_BLOB	          char[]
BLOB, TEXT	                MYSQL_TYPE_BLOB	                  char[]
MEDIUMBLOB, MEDIUMTEXT	    MYSQL_TYPE_MEDIUM_BLOB	          char[]
LONGBLOB, LONGTEXT	        MYSQL_TYPE_LONG_BLOB	          char[]
BIT	                        MYSQL_TYPE_BIT	                  char[]


// API调用例子如下：
// For example, bind the result buffers for all 4 columns before fetching them
MYSQL_BIND    bind[4];
MYSQL_TIME    ts;
unsigned long length[4];
short         small_data;
int           int_data;
char          str_data[STRING_SIZE];
my_bool       is_null[4];
my_bool       error[4];
memset(bind, 0, sizeof(bind));

// INTEGER COLUMN 
bind[0].buffer_type= MYSQL_TYPE_LONG;
bind[0].buffer= (char *)&int_data;
bind[0].is_null= &is_null[0];
bind[0].length= &length[0];
bind[0].error= &error[0];

// STRING COLUMN
bind[1].buffer_type= MYSQL_TYPE_STRING;
bind[1].buffer= (char *)str_data;
bind[1].buffer_length= STRING_SIZE;
bind[1].is_null= &is_null[1];
bind[1].length= &length[1];
bind[1].error= &error[1];

// SMALLINT COLUMN
bind[2].buffer_type= MYSQL_TYPE_SHORT;
bind[2].buffer= (char *)&small_data;
bind[2].is_null= &is_null[2];
bind[2].length= &length[2];
bind[2].error= &error[2];

// TIMESTAMP COLUMN
bind[3].buffer_type= MYSQL_TYPE_TIMESTAMP;
bind[3].buffer= (char *)&ts;
bind[3].is_null= &is_null[3];
bind[3].length= &length[3];
bind[3].error= &error[3];
*/


#ifndef CMYSQL_H
#define CMYSQL_H

#include <mysql/my_global.h>
#include <mysql/my_sys.h>
#include <mysql/mysql.h>

#include "base/MacroDefine.h"
#include "base/ErrorCode.h"


namespace NDBOpt
{

#if 0
// Mysql 表列字段数据定义
typedef struct st_mysql_field
{
    char *name;                 /* Name of column */
    char *org_name;             /* Original column name, if an alias */
    char *table;                /* Table of column if column was a field */
    char *org_table;            /* Org table name, if table was an alias */
    char *db;                   /* Database for table */
    char *catalog;              /* Catalog for table */
    char *def;                  /* Default value (set by mysql_list_fields) */
    unsigned long length;       /* Width of column (create length) */
    unsigned long max_length;   /* Max width for selected set */
    unsigned int name_length;
    unsigned int org_name_length;
    unsigned int table_length;
    unsigned int org_table_length;
    unsigned int db_length;
    unsigned int catalog_length;
    unsigned int def_length;
    unsigned int flags;         /* Div flags */
    unsigned int decimals;      /* Number of decimals in field */
    unsigned int charsetnr;     /* Character set */
    enum enum_field_types type; /* Type of field. See mysql_com.h for types */
    void *extension;
} MYSQL_FIELD;

// Mysql 预处理执行器绑定数据类型
typedef struct st_mysql_bind
{
    unsigned long *length;          /* output length pointer */
    my_bool       *is_null;         /* Pointer to null indicator */
    void      *buffer;              /* buffer to get/put data */
	
	/* set this if you want to track data truncations happened during fetch */
    my_bool       *error;
    unsigned char *row_ptr;         /* for the current data position */
    void (*store_param_func)(NET *net, struct st_mysql_bind *param);
    void (*fetch_result)(struct st_mysql_bind *, MYSQL_FIELD *,
                         unsigned char **row);
    void (*skip_result)(struct st_mysql_bind *, MYSQL_FIELD *,
                unsigned char **row);
				
	/* output buffer length, must be set when fetching str/binary */
    unsigned long buffer_length;
    unsigned long offset;           /* offset position for char/binary fetch */
    unsigned long length_value;     /* Used if length is 0 */
    unsigned int  param_number;     /* For null count and error messages */
    unsigned int  pack_length;      /* Internal length for packed data */
    enum enum_field_types buffer_type;    /* buffer type */
    my_bool       error_value;      /* used if error is 0 */
    my_bool       is_unsigned;      /* set if integer type is unsigned */
    my_bool   long_data_used;       /* If used with mysql_send_long_data */
    my_bool   is_null_value;        /* Used if is_null is 0 */
    void *extension;
} MYSQL_BIND;

typedef char **MYSQL_ROW;           /* return data as array of strings */

#endif  // #if 0





// Mysql 表行数据定义
// typedef char **MYSQL_ROW;  /* return data as array of strings */
typedef MYSQL_ROW RowDataPtr;

// Mysql 表列字段数据定义
typedef MYSQL_FIELD* FieldDataPtr;

// Mysql 预处理执行器绑定数据类型
typedef MYSQL_BIND BindData;

class CDBOpertaion;

// ID&Name 最大长度
static const unsigned int MaxNameLen = 128;


// 查询结果集合数据
class CQueryResult
{
public:
    CQueryResult();
	~CQueryResult();
	
public:
    void reset(CDBOpertaion* dbOpt = NULL, MYSQL_RES* result = NULL, int status = 0);
	
public:
	RowDataPtr getNextRow();
	unsigned long getRowCount();
	
public:
	FieldDataPtr getNextField();
	FieldDataPtr getAllFields();
	FieldDataPtr getField(unsigned int idx);
	unsigned int getFieldCount();
	unsigned long* getFieldLengthsOfCurrentRow();
	
public:
    // 1）释放当前结果集数据，必须释放当前结果集才能取下一个结果集
    // 2）是否存在下一个结果集，多语句执行或者存储过程执行有可能会返回多个结果集；
	bool nextResult();
	
private:
	void freeResult();
	
	CQueryResult(CDBOpertaion* dbOpt, MYSQL_RES* result, int status);

private:
    CDBOpertaion* m_dbOpt;
	MYSQL_RES* m_result;
    int m_resultStatus;


    friend class CDBOpertaion;

DISABLE_COPY_ASSIGN(CQueryResult);
};


// 预处理执行器，加快执行性能
class CPreparedStmt
{
private:
	CPreparedStmt(CDBOpertaion* dbOpt, MYSQL_STMT* stmt);
	~CPreparedStmt();
	
	// 操作错误信息
public:
    unsigned int errorNum();
	const char* errorInfo();
	const char* stateInfo();

    // 执行sql命令操作
public:
    void setResultMode(char mode);  // mode参数 0：一次取回所有查询的结果集数据；非0：结果集分执行动作每次取回一个； 默认值为0
	int prepare(const char* stmtStr, unsigned long length);
	bool bindParam(BindData* bind);
	bool bindResult(BindData* bind);
	int execute();
	bool reset();
	bool nextResult();  // 存储过程调用，存在多个结果集
    bool freeResult();  // 释放prepare结果集资源

	// 结果集行数据
public:
	bool toNextRow();
	void seekToRow(const unsigned long long row);
	unsigned long getAffectedRows();
	unsigned long getRowCount();

public:
	unsigned int getFieldCount();
	unsigned long getParamCount();
	

private:
    CDBOpertaion* m_dbOpt;
	MYSQL_STMT* m_stmt;
	char m_resultMode;


    friend class CDBOpertaion;

DISABLE_CONSTRUCTION_ASSIGN(CPreparedStmt);
};


// 注意：DB操作非线程安全！
// 单个数据库操作，每个实例对应操作一个数据库
class CDBOpertaion
{
public:
	CDBOpertaion(const char* host, const char* user, const char* db);
	~CDBOpertaion();
	
	// 操作错误信息
public:
    unsigned int errorNum();
	const char* errorInfo();
	const char* stateInfo();

    // 执行任意合法的sql语句
public:
	int executeSQL(const char* sql);
	int executeSQL(const char* sql, const unsigned int len);
	
    // 创建&删除 数据库表
public:
    int createTable(const char* sql);
	int createTable(const char* sql, const unsigned int len);
	int dropTable(const char* tableName);
	
	// 修改数据库表数据
public:
    int modifyTable(const char* sql);
	int modifyTable(const char* sql, const unsigned int len);
	unsigned long getAffectedRows();
	
public:
    // 查询数据库表，一次性获取全部查询结果集合
    int queryTableAllResult(const char* sql, CQueryResult& qResult);
	int queryTableAllResult(const char* sql, const unsigned int len, CQueryResult& qResult);
	
	int queryTableAllResult(const char* sql, CQueryResult*& qResult);
	int queryTableAllResult(const char* sql, const unsigned int len, CQueryResult*& qResult);

	// 查询数据库表，需要依次获取查询结果集合
    int queryTableResult(const char* sql, CQueryResult& qResult);
	int queryTableResult(const char* sql, const unsigned int len, CQueryResult& qResult);
	
	int queryTableResult(const char* sql, CQueryResult*& qResult);
	int queryTableResult(const char* sql, const unsigned int len, CQueryResult*& qResult);
	
	// 使用完查询结果集对象后，一定记得释放该对象，否则内存泄露
	void releaseQueryResult(CQueryResult*& qResult);

    // 一次执行多条语句
public:
	int executeMultiSql(const char* sql, CQueryResult& qResult);
	int executeMultiSql(const char* sql, const unsigned int len, CQueryResult& qResult);
	
	int executeMultiSql(const char* sql, CQueryResult*& qResult);
	int executeMultiSql(const char* sql, const unsigned int len, CQueryResult*& qResult);
	
	// 预处理执行器
public:
	int createPreparedStmt(CPreparedStmt*& preStmt);
	void releasePreparedStmt(CPreparedStmt*& preStmt);
	
	// 事务处理相关，一般情况下执行事务都使用sql语句执行实现而不调用API
public:
	bool autoCommit(char mode);
	bool commit();
	bool rollback();
	
	// 其他操作
public:
	bool realEscapeString(char* to, const char* from, unsigned long& length);  // 创建可在SQL语句中使用的合法SQL字符串

    // 数据库连接基本信息
public:
	const char* getHost();
	const char* getUser();
	const char* getDBName();
	
    MYSQL* getHandler();
	
private:
	// 查询数据库表，一次性获取全部查询结果集合
	int queryTableAllResult(const char* sql, const unsigned int len, MYSQL_RES*& result);
	
	// 查询数据库表，需要依次获取查询结果集合
	int queryTableResult(const char* sql, const unsigned int len, MYSQL_RES*& result);
	
private:
    char m_host[MaxNameLen];
	char m_user[MaxNameLen];
	char m_dbName[MaxNameLen];
	
    MYSQL m_mysqlHandler;


DISABLE_CONSTRUCTION_ASSIGN(CDBOpertaion);
};


// MySql相关操作
class CMySql
{
public:
    static int createDBOpt(CDBOpertaion*& dbOpt, const char* host, const char* user, const char* passwd, const char* db, unsigned int port = 0);
	static void destroyDBOpt(CDBOpertaion*& dbOpt);

private:
    static CMySql& instance();
	
private:
	CMySql();
	~CMySql();
	
private:
	bool m_isInitOk;


DISABLE_COPY_ASSIGN(CMySql);
};

}

#endif // CMYSQL_H
