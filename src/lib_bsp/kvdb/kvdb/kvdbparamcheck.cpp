#include "kvdbparamcheck.h"
#include "json/json.h"
#include "eventservice/util/base64.h"

KvdbParamCheck::KvdbParamCheck() {
}

KvdbParamCheck::~KvdbParamCheck() {
}

KvdbParamCheck* KvdbParamCheck::getIntance() {
  static KvdbParamCheck* pthiz = NULL;
  if (NULL == pthiz) {
    pthiz = new KvdbParamCheck();
  }
  return pthiz;
}

bool KvdbParamCheck::check_param_valid(const std::string &cfg_value, const std::string &match_value) {
  Json::Reader jread;
  Json::Value j_paramin, j_vaildparam;

  if(!jread.parse(match_value, j_vaildparam)) {
    DLOG_WARNING(MOD_EB, "validparam parse fail : %s.\n", match_value.c_str());
    return false;
  }

  if(!jread.parse(cfg_value, j_paramin)) {
    // 解析不出，可能是只有个字符串，不是json格式，只处理json的，其他格式模块自己校验
    // 也有可能是文件本身有问题，返回false，使用默认参数
    DLOG_WARNING(MOD_EB, "sparamin parse fail : %s.\n", cfg_value.c_str());
    return false;
  }

  // 是json 则所有元素都进行判断，分为一般模式和正则表达式模式
  // 正则表达式消耗内存过大，暂时不使用
  return check_param_valid_nomal(j_paramin, j_vaildparam);
  /*
  if (j_vaildparam.isMember(JKEY_CHECKMODE) && j_vaildparam[JKEY_CHECKMODE].asInt() == CHECK_MODE_0) {
    return check_param_valid_nomal(j_paramin, j_vaildparam);
  } else if (j_vaildparam.isMember(JKEY_CHECKMODE) && j_vaildparam[JKEY_CHECKMODE].asInt() == CHECK_MODE_1) {
    return check_param_valid_regex(j_paramin, j_vaildparam);
  } else {
    return false;
  }
  */
}

// 检查json 普通方式
bool KvdbParamCheck::check_param_valid_nomal(Json::Value j_paramin, Json::Value j_vaildparam) {
  Json::Value::Members members = j_paramin.getMemberNames();
  for (uint32 i = 0; i < members.size(); i++) {
    if (false == j_vaildparam.isMember(members[i])) {
      DLOG_WARNING(MOD_EB, "%s.\n", members[i].c_str());
      return false;
    }

    if (Json::objectValue == j_paramin[members[i]].type()) {
      bool bret = check_param_valid_nomal(j_paramin[members[i]], j_vaildparam[members[i]]);
      if (false == bret) {
        DLOG_WARNING(MOD_EB, "");
        return false;
      }
    }

    if (Json::arrayValue == j_paramin[members[i]].type()) {
      if (Json::arrayValue != j_vaildparam[members[i]].type()) {
        DLOG_WARNING(MOD_EB, "");
        return false;
      }

      uint32 nreq = j_paramin[members[i]].size();

      for (uint32 idx = 0; idx < nreq; idx++) {
        bool bret = check_param_valid_nomal(j_paramin[members[i]][idx],j_vaildparam[members[i]][0]);
        if (false == bret) {
          DLOG_WARNING(MOD_EB, "");
          return false;
        }
      }
    }

    if (Json::nullValue == j_paramin[members[i]].type()) {
      int32 type = j_vaildparam[members[i]][JKEY_TYPE].asInt();
      if (type >= (KVDB_PARAM_TYPE_INT + PARAM_TYPE_ELSE) && type < (KVDB_PARAM_TYPE_MAX + PARAM_TYPE_ELSE)) {
        continue;
      } else {
        DLOG_WARNING(MOD_EB, "param is null !");
        return false;
      }
    }

    // json读出来数据是整型数字的一般都是intValue
    if (Json::intValue == j_paramin[members[i]].type()) {
      int32 type = j_vaildparam[members[i]][JKEY_TYPE].asInt();
      if (type == KVDB_PARAM_TYPE_INT || type == (KVDB_PARAM_TYPE_INT + PARAM_TYPE_ELSE)
          || type == KVDB_PARAM_TYPE_ENUM || type == (KVDB_PARAM_TYPE_ENUM + PARAM_TYPE_ELSE)) {
        int32 nparam = j_paramin[members[i]].asInt();
        if (nparam >= j_vaildparam[members[i]][JKEY_MIN].asInt() && nparam <= j_vaildparam[members[i]][JKEY_MAX].asInt()) {
          continue;
        } else {
          DLOG_WARNING(MOD_EB, "match intValue fail!");
          return false;
        }
      } else if (type == KVDB_PARAM_TYPE_FLOAT || type == (KVDB_PARAM_TYPE_FLOAT + PARAM_TYPE_ELSE)) {
        int32 nparam = j_paramin[members[i]].asInt();
        if (nparam >= j_vaildparam[members[i]][JKEY_MIN].asFloat() && nparam <= j_vaildparam[members[i]][JKEY_MAX].asFloat()) {
          continue;
        } else {
          DLOG_WARNING(MOD_EB, "match intValue fail!");
          return false;
        }
      } else {
        DLOG_WARNING(MOD_EB, "param is not intValue !");
        return false;
      }
    }

    if (Json::realValue == j_paramin[members[i]].type()) {
      int32 type = j_vaildparam[members[i]][JKEY_TYPE].asInt();
      if (type == KVDB_PARAM_TYPE_FLOAT || type == (KVDB_PARAM_TYPE_FLOAT + PARAM_TYPE_ELSE)) {
        int32 nparam = j_paramin[members[i]].asFloat();
        if (nparam >= j_vaildparam[members[i]][JKEY_MIN].asFloat()
            && nparam <= j_vaildparam[members[i]][JKEY_MAX].asFloat()) {
          continue;
        } else {
          DLOG_WARNING(MOD_EB, "match intValue fail!");
          return false;
        }
      } else {
        DLOG_WARNING(MOD_EB, "param is not realValue !");
        return false;
      }
    }

    if (Json::stringValue == j_paramin[members[i]].type()) {
      int32 type = j_vaildparam[members[i]][JKEY_TYPE].asInt();
      if (type == KVDB_PARAM_TYPE_STRING || type == (KVDB_PARAM_TYPE_STRING + PARAM_TYPE_ELSE)) {
        int32 nslen = j_paramin[members[i]].asString().size();
        if (nslen >= j_vaildparam[members[i]][JKEY_MINLEN].asInt() && nslen <= j_vaildparam[members[i]][JKEY_MAXLEN].asInt()) {
          continue;
        } else {
          DLOG_WARNING(MOD_EB, "match stringValue fail!");
          return false;
        }
      } else {
        DLOG_WARNING(MOD_EB, "param is not stringValue !");
        return false;
      }
    }

    if (Json::booleanValue == j_paramin[members[i]].type()) {
      int32 type = j_vaildparam[members[i]][JKEY_TYPE].asInt();
      if (type == KVDB_PARAM_TYPE_BOOL || type == (KVDB_PARAM_TYPE_BOOL + PARAM_TYPE_ELSE)) {
        bool bparam = j_paramin[members[i]].asBool();
        if (bparam == j_vaildparam[members[i]][JKEY_MAX].asBool() || bparam == j_vaildparam[members[i]][JKEY_MIN].asBool()) {
          continue;
        } else {
          DLOG_WARNING(MOD_EB, "match stringValue fail!");
          return false;
        }
      } else {
        DLOG_WARNING(MOD_EB, "param is not stringValue !");
        return false;
      }
    }
  }

  return true;
}

// 正则表达式消耗内存过大，暂时不使用
// 检查json 正则表达式方式
#if 0
bool KvdbParamCheck::check_param_valid_regex(Json::Value j_paramin, Json::Value j_vaildparam) {
  Json::Value::Members members = j_paramin.getMemberNames();
  for (uint32 i = 0; i < members.size(); i++) {
    if (false == j_vaildparam.isMember(members[i])) {
      DLOG_WARNING(MOD_EB, "%s.\n", members[i].c_str());
      return false;
    }

    if (Json::objectValue == j_paramin[members[i]].type()) {
      bool bret = check_param_valid_regex(j_paramin[members[i]], j_vaildparam[members[i]]);
      if (false == bret) {
        DLOG_WARNING(MOD_EB, "");
        return false;
      }
    }

    if (Json::arrayValue == j_paramin[members[i]].type()) {
      if (Json::arrayValue != j_vaildparam[members[i]].type()) {
        DLOG_WARNING(MOD_EB, "");
        return false;
      }

      uint32 nreq = j_paramin[members[i]].size();

      for (uint32 idx = 0; idx < nreq; idx++) {
        bool bret = check_param_valid_regex(j_paramin[members[i]][idx],j_vaildparam[members[i]][0]);
        if (false == bret) {
          DLOG_WARNING(MOD_EB, "");
          return false;
        }
      }
    }

    if (Json::nullValue == j_paramin[members[i]].type()) {
      if(regex_match("", std::regex(j_vaildparam[members[i]].asString()))) {
        continue;
      } else {
        DLOG_WARNING(MOD_EB, "nullValue match fail members : %s", members[i].c_str());
        return false;
      }
    }

    if (Json::intValue == j_paramin[members[i]].type()) {
      if(regex_match(std::to_string(j_paramin[members[i]].asInt()), std::regex(j_vaildparam[members[i]].asString()))) {
        continue;
      } else {
        DLOG_WARNING(MOD_EB, "intValue match fail members : %s", members[i].c_str());
        return false;
      }
    }

    if (Json::uintValue == j_paramin[members[i]].type()) {
      if(regex_match(std::to_string(j_paramin[members[i]].asUInt()), std::regex(j_vaildparam[members[i]].asString()))) {
        continue;
      } else {
        DLOG_WARNING(MOD_EB, "uintValue match fail members : %s", members[i].c_str());
        return false;
      }
    }

    if (Json::realValue == j_paramin[members[i]].type()) {
      if (regex_match(std::to_string(j_paramin[members[i]].asDouble()), std::regex(j_vaildparam[members[i]].asString()))
          || regex_match(std::to_string(j_paramin[members[i]].asFloat()), std::regex(j_vaildparam[members[i]].asString()))) {
        continue;
      } else {
        DLOG_WARNING(MOD_EB, "realValue match fail members : %s", members[i].c_str());
        return false;
      }
    }

    if (Json::stringValue == j_paramin[members[i]].type()) {
      if(regex_match(j_paramin[members[i]].asString(), std::regex(j_vaildparam[members[i]].asString()))) {
        continue;
      } else {
        DLOG_WARNING(MOD_EB, "stringValue match fail members : %s", members[i].c_str());
        return false;
      }
    }

    if (Json::booleanValue == j_paramin[members[i]].type()) {
      if(regex_match(std::to_string(j_paramin[members[i]].asBool()), std::regex(j_vaildparam[members[i]].asString()))) {
        continue;
      } else {
        DLOG_WARNING(MOD_EB, "booleanValue match fail members : %s", members[i].c_str());
        return false;
      }
    }
  }

  return true;
}
#endif

// 提取默认配置
std::string KvdbParamCheck::get_default_param(std::string &validparam) {
  std::string sret;
  Json::Reader jread;
  Json::Value j_vaildparam;

  if(!jread.parse(validparam, j_vaildparam)) {
    DLOG_WARNING(MOD_EB, "get_default_param validparam parse fail : %s.\n", validparam.c_str());
    return "";
  }

  Json::Value::Members members = j_vaildparam.getMemberNames();
  for (uint32 i = 0; i < members.size(); i++) {
    if (members[i] == JKEY_DEFAULT) {
      Json2String(j_vaildparam[JKEY_DEFAULT], sret);
      return sret;
    }
  }

  return "";
}
