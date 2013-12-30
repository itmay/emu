#include <mt/env/asioStub/tcp/socket.hpp>
#include <mt/env/asioStub/ioService.hpp>
#include <glog/logging.h>

namespace eMU
{
namespace mt
{
namespace env
{
namespace asioStub
{
namespace ip
{
namespace tcp
{

socket::socket(io_service &ioService):
    BaseSocket(ioService) {}

void socket::close()
{

}

bool socket::is_open() const
{
    return true;
}

void socket::shutdown(boost::asio::ip::tcp::socket::shutdown_type type) {}

void socket::async_receive(const boost::asio::mutable_buffers_1 &buffer, const io_service::IoHandler &handler)
{
    receiveBuffer_ = buffer;
    receiveHandler_ = handler;
}

void socket::async_send(const boost::asio::mutable_buffers_1 &buffer, const io_service::IoHandler &handler)
{
    sendBuffer_ = buffer;
    sendHandler_ = handler;
}

void socket::connect(const boost::asio::ip::tcp::endpoint &endpoint, boost::system::error_code& errorCode)
{

}

void socket::disconnect()
{
    receiveHandler_(boost::asio::error::eof, 0);
}

}
}
}
}
}
}
