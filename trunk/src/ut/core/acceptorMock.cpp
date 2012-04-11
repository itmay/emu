#include "acceptorMock.hpp"

namespace eMUNetworkUT = eMU::ut::network;

eMUNetworkUT::acceptorMock_t::acceptorMock_t(ioServiceStub_t &ioService,
                                             const boost::asio::ip::tcp::endpoint &endpoint) {
    ON_CALL(*this, async_accept(::testing::_,
                                ::testing::_)).WillByDefault(::testing::Invoke(this,
                                                                               &acceptorMock_t::impl_async_accept));
}

void eMUNetworkUT::acceptorMock_t::impl_async_accept(socketMock_t &socket, const acceptHandler_t &handler) {
    acceptHandler_ = handler;
    socket_ = &socket;
}

void eMUNetworkUT::acceptorMock_t::expectCall_async_accept() {
    EXPECT_CALL(*this, async_accept(::testing::_, ::testing::_));
}