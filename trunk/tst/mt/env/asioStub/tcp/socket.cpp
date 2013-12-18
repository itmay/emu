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
    BaseSocket(ioService),
    openState_(true) {}

void socket::close()
{
    ioService_.closeTcpConnection(this);
}

bool socket::is_open() const
{
    return openState_;
}

void socket::shutdown(boost::asio::ip::tcp::socket::shutdown_type type) {}

void socket::async_receive(const boost::asio::mutable_buffers_1 &buffer, const IoHandler &handler)
{
    receiveBuffer_ = buffer;
    receiveHandler_ = handler;
}

void socket::async_send(const boost::asio::mutable_buffers_1 &buffer, const IoHandler &handler)
{
    sendBuffer_ = buffer;
    sendHandler_ = handler;
}

void socket::connect(const boost::asio::ip::tcp::endpoint &endpoint, boost::system::error_code& errorCode)
{
    if(!ioService_.establishClientTcpSocket(*this))
    {
        errorCode = boost::asio::error::access_denied;
    }
}

boost::asio::ip::tcp::endpoint socket::remote_endpoint() const
{
    return boost::asio::ip::tcp::endpoint();
}

void socket::disconnect()
{
    receiveHandler_(boost::asio::error::eof, 0);
}

void socket::setOpenState(bool state)
{
    openState_ = state;
}

}
}
}
}
}
}