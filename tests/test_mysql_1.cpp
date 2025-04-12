#include <mysql/mysql.h>
#include <iostream>
#include <cstdlib>
#include <unistd.h>

const std::string host = "127.0.0.1";
const std::string user = "lzh";
const std::string password = "1";
const std::string db = "lzh";
const int port = 3306;

int test_1() {
       // 1. 初始化 MySQL 连接句柄
       MYSQL* conn = mysql_init(nullptr);
       if (conn == nullptr) {
           std::cerr << "mysql_init() failed\n";
           return EXIT_FAILURE;
       }
   
       // 2. 可选：设置连接选项，例如字符集
       if(mysql_options(conn, MYSQL_SET_CHARSET_NAME, "utf8mb4")) {
           std::cerr << "设置字符集失败: " << mysql_error(conn) << "\n";
       }
   
       // 3. 连接数据库
       const char* host = "127.0.0.1";       // 主机地址
       const char* user = "lzh";            // 用户名
       const char* passwd = "1"; // 密码，请替换为你实际密码
       const char* db = "lzh";           // 要连接的数据库（可选），若为空则不指定
       unsigned int port = 3;             // MySQL 默认端口
   
       if(mysql_real_connect(conn, host, user, passwd, db, port, nullptr, 0) == nullptr) {
           std::cerr << "mysql_real_connect() failed: " 
                     << mysql_error(conn) << "\n";
           mysql_close(conn);
           return EXIT_FAILURE;
       }
   
       std::cout << "连接 MySQL 成功！" << std::endl;
   
       // 4. 执行一个简单查询：显示所有数据库
       if(mysql_query(conn, "SHOW DATABASES")) {
           std::cerr << "查询失败: " << mysql_error(conn) << "\n";
       } else {
           MYSQL_RES* result = mysql_store_result(conn);
           if(result) {
               MYSQL_ROW row;
               std::cout << "数据库列表：" << std::endl;
               while((row = mysql_fetch_row(result))) {
                   std::cout << row[0] << std::endl;
               }
               mysql_free_result(result);
           } else {
               std::cerr << "mysql_store_result() 失败: " << mysql_error(conn) << "\n";
           }
       }
   
       // 5. 关闭数据库连接
       mysql_close(conn);
   
       return EXIT_SUCCESS;
}
int test_2() {
    printf("mysql client Version: %s\n", mysql_get_client_info());
    return 0;
}

void test_3() {
    //printf("mysql client Version: %s\n", mysql_get_client_info());
    MYSQL* my = mysql_init(nullptr);
    if(nullptr == my)
    {
        std::cerr << "init MYSQL error" << std::endl;
    }
 
    if(mysql_real_connect(my,host.c_str(),user.c_str(),password.c_str(),db.c_str(),port,nullptr,0) == nullptr)
    {
        std::cerr << "mysql connect error" << std::endl;
    }
 
    std::cout<< "mysql connect success" << std::endl;
    sleep(5);
 
    mysql_close(my);

}
int main() {
    test_3();
    return 0;
}
