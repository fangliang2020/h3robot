#ifndef STP_ACCESSKEY_H_
#define STP_ACCESSKEY_H_

#include <string>
#include "eventservice/base/basictypes.h"

#define TEMP_ACCESSKEY 1
#define MAIN_ACCESSKEY 2
#define DEVICE_ACCESSKEY 3

namespace chml {

std::string CreateAccessKeyId(uint8 type, uint32 uid, uint32 expires);

std::string CreateAccessKeyId(uint8 type, const std::string &sn);

std::string CreateAccessKeySecret(const std::string &id);

bool CheckAccessKeyId(const std::string &id);

bool CheckAccessKeySecret(const std::string &secret);

bool CheckAccessKey(const std::string &id, const std::string &secret);

uint8 GetAccessKeyType(const std::string &id);

uint32 GetAccessKeyUid(const std::string &id);

uint32 GetAccessKeyExpires(const std::string &id);

std::string GetAccessKeySN(const std::string &id);
}

#endif //STP_ACCESSKEY_H_
