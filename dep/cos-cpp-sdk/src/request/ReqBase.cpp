#include "request/ReqBase.h"
#include "util/HttpUtil.h"
#include "util/FileUtil.h"
#include "Auth.h"
#include <string>

namespace qcloud_cos{

string ReqBase::getBucket()
{
    return this->msBucket;
}

}

