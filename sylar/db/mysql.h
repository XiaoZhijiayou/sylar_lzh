#ifndef __SYLAR_DB_MYSQL_H__
#define __SYLAR_DB_MYSQL_H__

#include <mysql/mysql.h>
#include <memory>
#include <functional>
#include <map>
#include <vector>
#include "sylar/mutex.h"
#include "db.h"
#include "sylar/singleton.h"
#include <list>

// 为了适配类型
#ifndef my_bool
#define my_bool bool
#endif

namespace sylar {

//typedef std::shared_ptr<MYSQL_RES> MySQLResPtr;
//typedef std::shared_ptr<MYSQL> MySQLPtr;
class MySQL;
class MySQLStmt;
//class IMySQLUpdate {
//public:
//    typedef std::shared_ptr<IMySQLUpdate> ptr;
//    virtual ~IMySQLUpdate() {}
//    virtual int cmd(const char* format, ...) = 0;
//    virtual int cmd(const char* format, va_list ap) = 0;
//    virtual int cmd(const std::string& sql) = 0;
//    virtual std::shared_ptr<MySQL> getMySQL() = 0;
//};

struct MySQLTime {
    MySQLTime(time_t t)
        :ts(t) { }
    time_t ts;
};

bool mysql_time_to_time_t(const MYSQL_TIME& mt, time_t& ts);
bool time_t_to_mysql_time(const time_t& ts, MYSQL_TIME& mt);

// 普通的SQL语句查询后返回的结果集合
class MySQLRes : public ISQLData {
public:
    typedef std::shared_ptr<MySQLRes> ptr;
    typedef std::function<bool(MYSQL_ROW row
                ,int field_count, int row_no)> data_cb;
    MySQLRes(MYSQL_RES* res, int eno, const char* estr);

    MYSQL_RES* get() const { return m_data.get();}

    int getErrno() const override { return m_errno;}
    const std::string& getErrStr() const override { return m_errstr;}

    bool foreach(data_cb cb);

    int getDataCount() override;
    int getColumnCount() override;
    int getColumnBytes(int idx) override;
    int getColumnType(int idx) override;
    std::string getColumnName(int idx) override;

    bool isNull(int idx) override;
    int8_t getInt8(int idx) override;
    uint8_t getUint8(int idx) override;
    int16_t getInt16(int idx) override;
    uint16_t getUint16(int idx) override;
    int32_t getInt32(int idx) override;
    uint32_t getUint32(int idx) override;
    int64_t getInt64(int idx) override;
    uint64_t getUint64(int idx) override;
    float getFloat(int idx) override;
    double getDouble(int idx) override;
    std::string getString(int idx) override;
    std::string getBlob(int idx) override;
    time_t getTime(int idx) override;
    bool next() override;
private:
    int m_errno;
    std::string m_errstr;
    MYSQL_ROW m_cur;
    unsigned long* m_curLength;
    std::shared_ptr<MYSQL_RES> m_data;
};

// 用于包装通过预处理语句查询返回的结果集
class MySQLStmtRes : public ISQLData {
friend class MySQLStmt;
public:
    typedef std::shared_ptr<MySQLStmtRes> ptr;
    static MySQLStmtRes::ptr Create(std::shared_ptr<MySQLStmt> stmt);
    ~MySQLStmtRes();

    int getErrno() const override { return m_errno;}
    const std::string& getErrStr() const override { return m_errstr;}

    int getDataCount() override;
    int getColumnCount() override;
    int getColumnBytes(int idx) override;
    int getColumnType(int idx) override;
    std::string getColumnName(int idx) override;

    bool isNull(int idx) override;
    int8_t getInt8(int idx) override;
    uint8_t getUint8(int idx) override;
    int16_t getInt16(int idx) override;
    uint16_t getUint16(int idx) override;
    int32_t getInt32(int idx) override;
    uint32_t getUint32(int idx) override;
    int64_t getInt64(int idx) override;
    uint64_t getUint64(int idx) override;
    float getFloat(int idx) override;
    double getDouble(int idx) override;
    std::string getString(int idx) override;
    std::string getBlob(int idx) override;
    time_t getTime(int idx) override;
    bool next() override;
private:
    MySQLStmtRes(std::shared_ptr<MySQLStmt> stmt, int eno, const std::string& estr);
    struct Data {
        Data();
        ~Data();

        void alloc(size_t size);

        my_bool is_null;
        my_bool error;
        enum_field_types type;
        unsigned long length;
        int32_t data_length;
        char* data;
    };
private:
    int m_errno;
    std::string m_errstr;
    std::shared_ptr<MySQLStmt> m_stmt;
    std::vector<MYSQL_BIND> m_binds;
    std::vector<Data> m_datas;
};

class MySQLManager;
// 用于包装MySQL连接
class MySQL : public IDB
              ,public std::enable_shared_from_this<MySQL> {
friend class MySQLManager;
public:
    typedef std::shared_ptr<MySQL> ptr;

    MySQL(const std::map<std::string, std::string>& args);

    bool connect();
    bool ping();

    virtual int execute(const char* format, ...) override;
    int execute(const char* format, va_list ap);
    virtual int execute(const std::string& sql) override;
    int64_t getLastInsertId() override;
    std::shared_ptr<MySQL> getMySQL();
    std::shared_ptr<MYSQL> getRaw();

    uint64_t getAffectedRows();

    ISQLData::ptr query(const char* format, ...) override;
    ISQLData::ptr query(const char* format, va_list ap); 
    ISQLData::ptr query(const std::string& sql) override;

    ITransaction::ptr openTransaction(bool auto_commit) override;
    sylar::IStmt::ptr prepare(const std::string& sql) override;

    template<typename... Args>
    int execStmt(const char* stmt, Args&&... args);

    template<class... Args>
    ISQLData::ptr queryStmt(const char* stmt, Args&&... args);

    const char* cmd();

    bool use(const std::string& dbname);
    int getErrno() override;
    std::string getErrStr() override;
    uint64_t getInsertId();
private:
    bool isNeedCheck();
private:
    std::map<std::string, std::string> m_params;
    std::shared_ptr<MYSQL> m_mysql;

    std::string m_cmd;
    std::string m_dbname;

    uint64_t m_lastUsedTime;
    bool m_hasError;
    int32_t m_poolSize;
};

// 数据库事务操作
class MySQLTransaction : public ITransaction {
public:
    typedef std::shared_ptr<MySQLTransaction> ptr;

    // 构造一个预处理语句的结果对象
    static MySQLTransaction::ptr Create(MySQL::ptr mysql, bool auto_commit);
    ~MySQLTransaction();

    bool begin() override;
    bool commit() override;
    bool rollback() override;

    virtual int execute(const char* format, ...) override;
    int execute(const char* format, va_list ap);
    virtual int execute(const std::string& sql) override;
    int64_t getLastInsertId() override;
    std::shared_ptr<MySQL> getMySQL();

    bool isAutoCommit() const { return m_autoCommit;}
    bool isFinished() const { return m_isFinished;}
    bool isError() const { return m_hasError;}
private:
    MySQLTransaction(MySQL::ptr mysql, bool auto_commit);
private:
    MySQL::ptr m_mysql;
    bool m_autoCommit;
    bool m_isFinished;
    bool m_hasError;
};

// 表示一个预处理语句的封装对象，继承来自接口IStmt
class MySQLStmt : public IStmt
                  ,public std::enable_shared_from_this<MySQLStmt> {
public:
    typedef std::shared_ptr<MySQLStmt> ptr;
    static MySQLStmt::ptr Create(MySQL::ptr db, const std::string& stmt);

    ~MySQLStmt();
    int bind(int idx, const int8_t& value);
    int bind(int idx, const uint8_t& value);
    int bind(int idx, const int16_t& value);
    int bind(int idx, const uint16_t& value);
    int bind(int idx, const int32_t& value);
    int bind(int idx, const uint32_t& value);
    int bind(int idx, const int64_t& value);
    int bind(int idx, const uint64_t& value);
    int bind(int idx, const float& value);
    int bind(int idx, const double& value);
    int bind(int idx, const std::string& value);
    int bind(int idx, const char* value);
    int bind(int idx, const void* value, int len);
    //int bind(int idx, const MYSQL_TIME& value, int type = MYSQL_TYPE_TIMESTAMP);
    //for null type
    int bind(int idx);

    int bindInt8(int idx, const int8_t& value) override;
    int bindUint8(int idx, const uint8_t& value) override;
    int bindInt16(int idx, const int16_t& value) override;
    int bindUint16(int idx, const uint16_t& value) override;
    int bindInt32(int idx, const int32_t& value) override;
    int bindUint32(int idx, const uint32_t& value) override;
    int bindInt64(int idx, const int64_t& value) override;
    int bindUint64(int idx, const uint64_t& value) override;
    int bindFloat(int idx, const float& value) override;
    int bindDouble(int idx, const double& value) override;
    int bindString(int idx, const char* value) override;
    int bindString(int idx, const std::string& value) override;
    int bindBlob(int idx, const void* value, int64_t size) override;
    int bindBlob(int idx, const std::string& value) override;
    //int bindTime(int idx, const MYSQL_TIME& value, int type = MYSQL_TYPE_TIMESTAMP);
    int bindTime(int idx, const time_t& value) override;
    int bindNull(int idx) override;

    int getErrno() override;
    std::string getErrStr() override;

    int execute() override;
    int64_t getLastInsertId() override;
    ISQLData::ptr query() override;

    MYSQL_STMT* getRaw() const { return m_stmt;}
private:
    MySQLStmt(MySQL::ptr db, MYSQL_STMT* stmt);
private:
    MySQL::ptr m_mysql;
    MYSQL_STMT* m_stmt;
    std::vector<MYSQL_BIND> m_binds;
};

// 管理MySQL连接池，进行注册和获取数据库的连接
class MySQLManager {
public:
    typedef sylar::Mutex MutexType;

    MySQLManager();
    ~MySQLManager();

    MySQL::ptr get(const std::string& name);
    void registerMySQL(const std::string& name, const std::map<std::string, std::string>& params);

    // 定时检查连接健康状态
    void checkConnection(int sec = 30);

    uint32_t getMaxConn() const { return m_maxConn;}
    void setMaxConn(uint32_t v) { m_maxConn = v;}

    int execute(const std::string& name, const char* format, ...);
    int execute(const std::string& name, const char* format, va_list ap);
    int execute(const std::string& name, const std::string& sql);

    ISQLData::ptr query(const std::string& name, const char* format, ...);
    ISQLData::ptr query(const std::string& name, const char* format, va_list ap); 
    ISQLData::ptr query(const std::string& name, const std::string& sql);

    MySQLTransaction::ptr openTransaction(const std::string& name, bool auto_commit);
private:
    void freeMySQL(const std::string& name, MySQL* m);
private:
    uint32_t m_maxConn;
    MutexType m_mutex;
    std::map<std::string, std::list<MySQL*> > m_conns;
    std::map<std::string, std::map<std::string, std::string> > m_dbDefines;
};

// 静态工具类，封装了常用的数据库操作接口，简化了用户对常规数据库访问的方法
class MySQLUtil {
public:
    static ISQLData::ptr Query(const std::string& name, const char* format, ...);
    static ISQLData::ptr Query(const std::string& name, const char* format,va_list ap); 
    static ISQLData::ptr Query(const std::string& name, const std::string& sql);

    static ISQLData::ptr TryQuery(const std::string& name, uint32_t count, const char* format, ...);
    static ISQLData::ptr TryQuery(const std::string& name, uint32_t count, const std::string& sql);

    static int Execute(const std::string& name, const char* format, ...);
    static int Execute(const std::string& name, const char* format, va_list ap); 
    static int Execute(const std::string& name, const std::string& sql);

    static int TryExecute(const std::string& name, uint32_t count, const char* format, ...);
    static int TryExecute(const std::string& name, uint32_t count, const char* format, va_list ap); 
    static int TryExecute(const std::string& name, uint32_t count, const std::string& sql);
};

typedef sylar::Singleton<MySQLManager> MySQLMgr;

namespace {

// MySQLBinder基础模板，迭代终止版本
template<size_t N, typename... Args>
struct MySQLBinder {
    static int Bind(std::shared_ptr<MySQLStmt> stmt) { return 0; }
};

// MySQLBinder模板，用于递归绑定参数
template<typename... Args>
int bindX(MySQLStmt::ptr stmt, Args&... args) {
    return MySQLBinder<1, Args...>::Bind(stmt, args...);
}
}

// 执行处理语句，支持多个参数的绑定
template<typename... Args>
int MySQL::execStmt(const char* stmt, Args&&... args) {
    auto st = MySQLStmt::Create(shared_from_this(), stmt);
    if(!st) {
        return -1;
    }
    int rt = bindX(st, args...);
    if(rt != 0) {
        return rt;
    }
    return st->execute();
}

// 支持查询预处理语句
template<class... Args>
ISQLData::ptr MySQL::queryStmt(const char* stmt, Args&&... args) {
    auto st = MySQLStmt::Create(shared_from_this(), stmt);
    if(!st) {
        return nullptr;
    }
    int rt = bindX(st, args...);
    if(rt != 0) {
        return nullptr;
    }
    return st->query();
}

// 用于绑定参数的模板特化版本，用于递归绑定参数
namespace {

template<size_t N, typename Head, typename... Tail>
struct MySQLBinder<N, Head, Tail...> {
    static int Bind(MySQLStmt::ptr stmt
                    ,const Head&, Tail&...) {
        //static_assert(false, "invalid type");
        static_assert(sizeof...(Tail) < 0, "invalid type");
        return 0;
    }
};

#define XX(type, type2) \
template<size_t N, typename... Tail> \
struct MySQLBinder<N, type, Tail...> { \
    static int Bind(MySQLStmt::ptr stmt \
                    , type2 value \
                    , Tail&... tail) { \
        int rt = stmt->bind(N, value); \
        if(rt != 0) { \
            return rt; \
        } \
        return MySQLBinder<N + 1, Tail...>::Bind(stmt, tail...); \
    } \
};

//template<size_t N, typename... Tail>
//struct MySQLBinder<N, const char(&)[], Tail...> {
//    static int Bind(MySQLStmt::ptr stmt
//                    , const char value[]
//                    , const Tail&... tail) {
//        int rt = stmt->bind(N, (const char*)value);
//        if(rt != 0) {
//            return rt;
//        }
//        return MySQLBinder<N + 1, Tail...>::Bind(stmt, tail...);
//    }
//};

XX(char*, char*);
XX(const char*, char*);
XX(std::string, std::string&);
XX(int8_t, int8_t&);
XX(uint8_t, uint8_t&);
XX(int16_t, int16_t&);
XX(uint16_t, uint16_t&);
XX(int32_t, int32_t&);
XX(uint32_t, uint32_t&);
XX(int64_t, int64_t&);
XX(uint64_t, uint64_t&);
XX(float, float&);
XX(double, double&);
//XX(MYSQL_TIME, MYSQL_TIME&);
#undef XX
}
}

#endif
