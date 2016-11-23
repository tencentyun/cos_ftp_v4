#include "CosConfig.h"
#include <string>

using std::string;
namespace qcloud_cos {

uint64_t CosConfig::getAppid() const
{
    return this->appid;
}
string CosConfig::getSecretId() const
{
    return this->secret_id;
}
string CosConfig::getSecretKey() const
{
    return this->secret_key;
}

}
