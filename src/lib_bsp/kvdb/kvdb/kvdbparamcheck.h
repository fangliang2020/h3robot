#include "eventservice/base/basictypes.h"
#include "eventservice/log/log_client.h"

#define PARAM_TYPE_ELSE 10
#define CHECK_MODE_0    0
#define CHECK_MODE_1    1

#define JKEY_DEFAULT               "default"
#define JKEY_CHECKMODE             "checkmode"
#define JKEY_TYPE                  "type"
#define JKEY_MAX                   "max"
#define JKEY_MIN                   "min"
#define JKEY_MAXLEN                "maxlen"
#define JKEY_MINLEN                "minlen"

enum CheckParamType {
  KVDB_PARAM_TYPE_INT,
  KVDB_PARAM_TYPE_FLOAT,
  KVDB_PARAM_TYPE_STRING,
  KVDB_PARAM_TYPE_ENUM,
  KVDB_PARAM_TYPE_BOOL,
  KVDB_PARAM_TYPE_MAX,
};

// 校验类
class KvdbParamCheck {
public:
  KvdbParamCheck();
  ~KvdbParamCheck();

  static KvdbParamCheck* getIntance();

  bool check_param_valid(const std::string &cfg_value, const std::string &match_value);
  // 正则表达式消耗内存过大，暂时不使用
  // bool check_param_valid_regex(Json::Value j_paramin, Json::Value j_vaildparam);
  bool check_param_valid_nomal(Json::Value j_paramin, Json::Value j_vaildparam);
  std::string get_default_param(std::string &validparam);
};