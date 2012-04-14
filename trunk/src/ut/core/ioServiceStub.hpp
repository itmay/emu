#ifndef eMU_UT_IOSERVICESTUB_HPP
#define eMU_UT_IOSERVICESTUB_HPP

#include <boost/function.hpp>
#include <boost/system/error_code.hpp>

namespace eMU {
namespace ut {
namespace network {

class ioServiceStub_t: private boost::noncopyable {
public:
    typedef boost::function2<void, const boost::system::error_code&, size_t> ioHandler_t;

    class strand {
    public:
        strand(ioServiceStub_t &ioService) {}

        template<typename CompletionHandler>
        CompletionHandler wrap(const CompletionHandler &handler) {
            return handler;
        }

    private:
        strand();
    };

    template<typename CompletionHandler>
    void post(const CompletionHandler &handler) { handler(); }
};

}
}
}

#endif