#include <dataserver/server.hpp>
#include <mt/env/dataserver/database/sqlInterfaceStub.hpp>
#include <mt/env/testExceptionsCatch.hpp>

#include <protocol/dataserver/messageIds.hpp>
#include <protocol/dataserver/checkAccountRequest.hpp>
#include <protocol/dataserver/checkAccountResponse.hpp>
#include <protocol/dataserver/faultIndication.hpp>

#include <core/network/tcp/networkUser.hpp>

#include <gtest/gtest.h>

using eMU::dataserver::Server;
using eMU::dataserver::database::QueryResult;
using eMU::dataserver::database::Row;

using eMU::mt::env::asioStub::io_service;
using eMU::mt::env::dataserver::database::SqlInterfaceStub;

using eMU::protocol::ReadStream;
using eMU::protocol::dataserver::CheckAccountResult;
using eMU::protocol::dataserver::CheckAccountRequest;
using eMU::protocol::dataserver::CheckAccountResponse;
using eMU::protocol::dataserver::FaultIndication;
namespace MessageIds = eMU::protocol::dataserver::MessageIds;

using eMU::core::network::tcp::NetworkUser;

class DataserverTest: public ::testing::Test
{
protected:
    DataserverTest()
    {
        prepareConfiguration();
    }

    void prepareConfiguration()
    {
        configuration_.maxNumberOfUsers_ = 5;
        configuration_.port_ = 55960;
    }

    io_service ioService_;
    SqlInterfaceStub sqlInterface_;
    Server::Configuration configuration_;
};

TEST_F(DataserverTest, CheckAccountShoulBeSuccesful)
{
    Server server(ioService_, sqlInterface_, configuration_);
    server.startup();

    QueryResult queryResult;
    Row &row = queryResult.createRow(Row::Fields());
    CheckAccountResult checkAccountResult = CheckAccountResult::AccountInUse;
    row.insert(boost::lexical_cast<Row::Value>(static_cast<uint32_t>(checkAccountResult)));

    sqlInterface_.pushQueryResult(queryResult);
    sqlInterface_.pushQueryStatus(true);

    size_t connectionHash = ioService_.establishTcpConnection();

    NetworkUser::Hash clientHash(0x1234);
    IO_CHECK(ioService_.send(connectionHash, CheckAccountRequest(clientHash, "Account", "Password").getWriteStream()));

    const ReadStream &readStream = ioService_.receive(connectionHash);
    ASSERT_EQ(MessageIds::kCheckAccountResponse, readStream.getId());

    CheckAccountResponse response(readStream);
    ASSERT_EQ(clientHash, response.getClientHash());
    ASSERT_EQ(checkAccountResult, response.getResult());

    IO_CHECK(ioService_.disconnect(connectionHash));
}

TEST_F(DataserverTest, WhenQueryExecutionWasFailedThenFaultIndicationShouldBeReceived)
{
    Server server(ioService_, sqlInterface_, configuration_);
    server.startup();

    sqlInterface_.pushQueryStatus(false);

    size_t connectionHash = ioService_.establishTcpConnection();

    NetworkUser::Hash clientHash(0x1234);
    IO_CHECK(ioService_.send(connectionHash, CheckAccountRequest(clientHash, "Account", "Password").getWriteStream()));

    const ReadStream &readStream = ioService_.receive(connectionHash);
    ASSERT_EQ(MessageIds::kFaultIndication, readStream.getId());

    FaultIndication indication(readStream);
    ASSERT_EQ(clientHash, indication.getClientHash());

    IO_CHECK(ioService_.disconnect(connectionHash));
}

TEST_F(DataserverTest, WhenQueryResultIsEmptyThenFaultIndicationShouldBeReceived)
{
    Server server(ioService_, sqlInterface_, configuration_);
    server.startup();

    sqlInterface_.pushQueryStatus(true);
    sqlInterface_.pushQueryResult(QueryResult());

    size_t connectionHash = ioService_.establishTcpConnection();

    NetworkUser::Hash clientHash(0x1234);
    IO_CHECK(ioService_.send(connectionHash, CheckAccountRequest(clientHash, "Account", "Password").getWriteStream()));

    const ReadStream &readStream = ioService_.receive(connectionHash);
    ASSERT_EQ(MessageIds::kFaultIndication, readStream.getId());

    FaultIndication indication(readStream);
    ASSERT_EQ(clientHash, indication.getClientHash());

    IO_CHECK(ioService_.disconnect(connectionHash));
}

TEST_F(DataserverTest, WhenConnectionToDatabaseIsDiedThenFaultIndicationShouldBeSent)
{
    Server server(ioService_, sqlInterface_, configuration_);
    server.startup();

    sqlInterface_.setDied();

    size_t connectionHash = ioService_.establishTcpConnection();

    NetworkUser::Hash clientHash(0x1234);
    IO_CHECK(ioService_.send(connectionHash, CheckAccountRequest(clientHash, "Account", "Password").getWriteStream()));

    const ReadStream &readStream = ioService_.receive(connectionHash);
    ASSERT_EQ(MessageIds::kFaultIndication, readStream.getId());

    FaultIndication indication(readStream);
    ASSERT_EQ(clientHash, indication.getClientHash());

    IO_CHECK(ioService_.disconnect(connectionHash));
}