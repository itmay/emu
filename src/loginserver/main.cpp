#include <loginserver/main.hpp>
#include <loginserver/context.hpp>
#include <loginserver/protocol.hpp>
#include <loginserver/dataserverProtocol.hpp>
#include <loginserver/udpProtocol.hpp>
#include <core/network/tcp/connectionsAcceptor.hpp>
#include <core/common/xmlReader.hpp>
#include <core/common/concurrency.hpp>
#include <core/network/udp/connection.hpp>

#include <boost/thread.hpp>
#include <core/common/logging.hpp>
#include <gflags/gflags.h>

DEFINE_string(dataserver_host, "127.0.0.1", "Dataserver address");
DEFINE_int32(dataserver_port, 55960, "Dataserver port");
DEFINE_int32(max_users, 5, "Max number of users to connect");
DEFINE_int32(port, 55557, "server listen port");
DEFINE_int32(max_threads, 2, "max number of concurrent threads");


int main(int argsCount, char *args[])
{
    google::ParseCommandLineFlags(&argsCount, &args, true);

    eMU::loginserver::Context loginserverContext(FLAGS_max_users);
    eMU::core::common::XmlReader xmlReader(eMU::core::common::XmlReader::getXmlFileContent("./data/gameserversList.xml"));

    if(!loginserverContext.getGameserversList().initialize(xmlReader))
    {
        eMU_LOG(error) << "Initialization of gameservers list failed.";
        return 1;
    }

    eMU::loginserver::DataserverProtocol dataserverProtocol(loginserverContext);

    boost::asio::io_service ioService;
    eMU::core::network::tcp::Connection::Pointer dataserverConnection(new eMU::core::network::tcp::Connection(ioService, dataserverProtocol));
    if(!dataserverConnection->connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(FLAGS_dataserver_host),
                                                                    FLAGS_dataserver_port)))
    {
        eMU_LOG(error) << "Connection to datserver failed. host: " << FLAGS_dataserver_host << ", port: " << FLAGS_dataserver_port;
        return 1;
    }

    eMU::loginserver::UdpProtocol udpProtocol(loginserverContext);
    eMU::core::network::udp::Connection::Pointer udpConnection(new eMU::core::network::udp::Connection(ioService, FLAGS_port, udpProtocol));
    udpConnection->registerConnection();
    udpConnection->queueReceiveFrom();

    eMU::loginserver::Protocol loginserverProtocol(loginserverContext);
    eMU::core::network::tcp::ConnectionsAcceptor connectionsAcceptor(ioService, FLAGS_port, loginserverProtocol);
    connectionsAcceptor.queueAccept();

    eMU::core::common::Concurrency concurrency(ioService, FLAGS_max_threads);
    concurrency.start();
    concurrency.join();

    udpConnection->unregisterConnection();

    return 0;
}
