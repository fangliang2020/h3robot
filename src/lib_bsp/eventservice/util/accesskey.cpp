#include "accesskey.h"
#include <math.h>

#define TEST_APPID      "0123456789ABCDEFGHIJKLMNOPQRSTUV"
#define TEST_APPKEY     "0123456789abcdefghijklmnopqrstuv"
#define DEFAULT_START_TIME_STAMP    1527000000
#define MAX_PRINT_COUNT     62
#define APPID_APPKEY_SIZE   32
#define MAX_SIZTH_TWO_SIZE  128
#define START_TYPE_POS 8
#define START_ID_POS 9

static const uint8 MAP_ID_KEY[MAX_PRINT_COUNT] = {
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', // NOLINT
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', // NOLINT
  'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', // NOLINT
  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', // NOLINT
  'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z' // NOLINT
};

static const uint8 MAP_KEY_ID[MAX_SIZTH_TWO_SIZE] = {
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,
  0, 1, 2, 3, 4, 5, 6, 7,
  8, 9, 0, 0, 0, 0, 0, 0,
  0, 10, 11, 12, 13, 14, 15, 16,
  17, 18, 19, 20, 21, 22, 23, 24,
  25, 26, 27, 28, 29, 30, 31, 32,
  33, 34, 35, 0, 0, 0, 0, 0,
  0, 36, 37, 38, 39, 40, 41, 42,
  43, 44, 45, 46, 47, 48, 49, 50,
  51, 52, 53, 54, 55, 56, 57, 58,
  59, 60, 61, 0, 0, 0, 0, 0
};

const int MAX_RANDOM_KEY_SIZE = 62;
const char RANDOM_KEY[] = "abcdefghijkrmnopqlstuvwxyzABCDEFGHIJKRMNOPQLSTUVWXYZ0123456789"; // NOLINT

namespace chml {

std::string GetRandomString(std::size_t size) {
  std::string result;
  for (std::size_t i = 0; i < size; i++) {
    result.push_back(RANDOM_KEY[rand() % MAX_RANDOM_KEY_SIZE]);
  }
  return result;
}

const std::string DecToSixthTwoimal(uint32 owner_id) {
  std::string res;
  do {
    res.push_back(MAP_ID_KEY[owner_id % MAX_PRINT_COUNT]);
    owner_id = owner_id / MAX_PRINT_COUNT;
  } while (owner_id);
  return res;
}

std::string DataToMapKey(const char *data, uint32 size) {
  std::string result;
  for (uint32 i = 0; i < size; i += 2) {
    uint16_t value = *((uint16_t *) (data + i));
    result.push_back(MAP_ID_KEY[value % MAX_PRINT_COUNT]);
  }
  return result;
}

std::string ToCheckSumString(const char *data, uint32 size) {
  std::string result;
  for (uint32 i = 0; i < size; i += 3) {
    uint32 value = 0;
    value = data[i];
    value = value * 256 + data[i + 1];
    value = value * 256 + data[i + 2];
    result.push_back(MAP_ID_KEY[value % MAX_PRINT_COUNT]);
  }
  return result;
}

const std::string CreateCheckSumKey(const std::string base) {
  std::string data = DataToMapKey(base.c_str(), base.size()) + base;
  data += ToCheckSumString(data.c_str(), data.size());
  return data;
}

std::string CreateAccessKeyId(uint8 type, uint32 uid, uint32 expires) {
  std::string id_str = DecToSixthTwoimal(uid);
  std::string time_str = DecToSixthTwoimal(expires - DEFAULT_START_TIME_STAMP);
  std::string random_str;
  random_str.push_back(MAP_ID_KEY[type % MAX_PRINT_COUNT]);
  random_str.push_back(MAP_ID_KEY[id_str.size() % MAX_PRINT_COUNT]);
  random_str.append(id_str);
  random_str.push_back(MAP_ID_KEY[time_str.size() % MAX_PRINT_COUNT]);
  random_str.append(time_str);
  random_str += GetRandomString(16 - random_str.size());
  std::string accesskey_id = CreateCheckSumKey(random_str);
  return accesskey_id;
}

std::string CreateAccessKeyId(uint8 type, const std::string &sn) {
  if (sn.length() != 17)
    return "";
  uint32 sn1;
  uint32 sn2;
  sscanf(sn.c_str(), "%08x-%08x", &sn1, &sn2);

  std::string sn1_str = DecToSixthTwoimal(sn1);
  std::string sn2_str = DecToSixthTwoimal(sn2);
  std::string random_str;
  random_str.push_back(MAP_ID_KEY[type % MAX_PRINT_COUNT]);
  random_str.push_back(MAP_ID_KEY[sn1_str.size() % MAX_PRINT_COUNT]);
  random_str.append(sn1_str);
  random_str.push_back(MAP_ID_KEY[sn2_str.size() % MAX_PRINT_COUNT]);
  random_str.append(sn2_str);
  random_str += GetRandomString(16 - random_str.size());
  std::string accesskey_id = CreateCheckSumKey(random_str);
  return accesskey_id;
}

std::string CreateAccessKeySecret(const std::string &id) {
  int len = id.length();
  if (len != APPID_APPKEY_SIZE) {
    return "";
  }

  std::string accesskey_secret;
  if (id == TEST_APPID) {
    accesskey_secret = TEST_APPKEY;
  } else {
    std::string raccesskey_id;
    for (int i = len - 1; i >= 0; i--) {
      raccesskey_id.push_back(id.at(i));
    }

    accesskey_secret = CreateCheckSumKey(DataToMapKey(raccesskey_id.c_str(), raccesskey_id.size()));
  }

  return accesskey_secret;
}

bool CheckAccessKeyId(const std::string &id) {
  int len = id.length();
  if (len != APPID_APPKEY_SIZE) {
    return false;
  }

  if (id == TEST_APPID) {
    return true;
  }

  std::string check_id;
  for (std::size_t i = 0; i < 16; i++) {
    check_id.push_back(id[i + 8]);
  }
  std::string new_id = CreateCheckSumKey(check_id);
  return id == new_id;
}

bool CheckAccessKeySecret(const std::string &secret) {
  int len = secret.length();
  if (len != APPID_APPKEY_SIZE) {
    return false;
  }

  if (secret == TEST_APPKEY) {
    return true;
  }

  std::string check_secret;
  for (std::size_t i = 0; i < 16; i++) {
    check_secret.push_back(secret[i + 8]);
  }
  std::string new_secret = CreateCheckSumKey(check_secret);
  return secret == new_secret;
}

bool CheckAccessKey(const std::string &id, const std::string &secret) {
  if (id.length() != APPID_APPKEY_SIZE || secret.length() != APPID_APPKEY_SIZE) {
    return false;
  }

  if (id == TEST_APPID && secret == TEST_APPKEY) {
    return true;
  }

  std::string new_secret = CreateAccessKeySecret(id);
  return secret == new_secret;
}

uint8 GetAccessKeyType(const std::string &id) {
  if (id == TEST_APPID) {
    return 0;
  }

  if (!CheckAccessKeyId(id)) {
    return 0;
  }

  uint8 type = MAP_KEY_ID[(uint8) id[START_TYPE_POS]];
  return type;
}

uint32 GetAccessKeyUid(const std::string &id) {
  if (id == TEST_APPID) {
    return 0;
  }

  if (!CheckAccessKeyId(id)) {
    return 0;
  }

  uint8 size = MAP_KEY_ID[(uint8) id[START_ID_POS]];
  uint32 uid = 0;
  uint32 max_pos = START_ID_POS + size + 1;
  uint32 min_pos = START_ID_POS + 1;
  for (uint32 i = min_pos; i < max_pos; i++) {
    uint32 a = MAP_KEY_ID[(uint8) id[i]];
    uint32 b = (uint32) pow(MAX_PRINT_COUNT, i - min_pos);
    uid += a * b;
  }
  return uid;
}

uint32 GetAccessKeyExpires(const std::string &id) {
  if (id == TEST_APPKEY) {
    return DEFAULT_START_TIME_STAMP;
  }

  if (!CheckAccessKeyId(id)) {
    return 0;
  }

  uint8 size = MAP_KEY_ID[(uint8) id[START_ID_POS]];
  uint8 timestamp_size = MAP_KEY_ID[(uint8) id[START_ID_POS + size + 1]];
  uint32 timestamp = 0;
  uint32 max_pos = START_ID_POS + size + timestamp_size + 2;
  uint32 min_pos = START_ID_POS + size + 2;
  for (uint32 i = min_pos; i < max_pos; i++) {
    uint32 a = MAP_KEY_ID[(uint8) id[i]];
    uint32 b = (uint32) pow(MAX_PRINT_COUNT, i - min_pos);
    timestamp += a * b;
  }
  return timestamp + DEFAULT_START_TIME_STAMP;
}

std::string GetAccessKeySN(const std::string &id) {
  if (!CheckAccessKeyId(id))
    return "";

  uint32 sn1 = 0;
  {
    uint8 sn1_size = MAP_KEY_ID[(uint8) id[START_ID_POS]];
    uint32 max_pos = START_ID_POS + sn1_size + 1;
    uint32 min_pos = START_ID_POS + 1;
    for (uint32 i = min_pos; i < max_pos; i++) {
      uint32 a = MAP_KEY_ID[(uint8) id[i]];
      uint32 b = (uint32) pow(MAX_PRINT_COUNT, i - min_pos);
      sn1 += a * b;
    }
  }

  uint32 sn2 = 0;
  {
    uint8 sn1_size = MAP_KEY_ID[(uint8) id[START_ID_POS]];
    uint8 sn2_size = MAP_KEY_ID[(uint8) id[START_ID_POS + sn1_size + 1]];

    uint32 max_pos = START_ID_POS + sn1_size + sn2_size + 2;
    uint32 min_pos = START_ID_POS + sn1_size + 2;
    for (uint32 i = min_pos; i < max_pos; i++) {
      uint32 a = MAP_KEY_ID[(uint8) id[i]];
      uint32 b = (uint32) pow(MAX_PRINT_COUNT, i - min_pos);
      sn2 += a * b;
    }
  }

  char sn[18] = {0};
  sprintf((char *) sn, "%08x-%08x", sn1, sn2);
  return std::string(sn);
}

}


