#pragma once

#include <dataserver/database/sqlInterface.hpp>
#include <mysql/mysql.h>

namespace eMU
{
namespace dataserver
{
namespace database
{

class MySqlInterface: public SqlInterface
{
public:
    MySqlInterface();

    bool initialize();
    bool connect(const std::string &hostname, uint16_t port, const std::string &userName,
                 const std::string &password, const std::string &databaseName);

    void cleanup();
    std::string getErrorMessage();

    bool executeQuery(std::string query);
    QueryResult fetchQueryResult();

    void releaseQuery();
    bool isAlive();

private:
    Row::Fields fetchFields();
    void fetchRows(QueryResult &queryResult);

    MYSQL handle_;
    MYSQL_RES *queryResult_;
};

}
}
}
