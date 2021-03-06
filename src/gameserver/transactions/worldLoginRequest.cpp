#include <gameserver/transactions/worldLoginRequest.hpp>
#include <streaming/gameserver/worldLoginResponse.hpp>

#include <core/common/logging.hpp>

namespace eMU
{
namespace gameserver
{
namespace transactions
{

WorldLoginRequest::WorldLoginRequest(User &user, const streaming::gameserver::WorldLoginRequest &request):
    user_(user),
    request_(request) {}

bool WorldLoginRequest::isValid() const
{
    return true;
}

void WorldLoginRequest::handleValid()
{
    eMU_LOG(info) << "hash: " << user_.getHash() << ", accountId: " << user_.getAccountId();

    user_.getConnection().send(streaming::gameserver::WorldLoginResponse(0).getWriteStream().getPayload());
}

void WorldLoginRequest::handleInvalid()
{
}

}
}
}
