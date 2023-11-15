#include "yamlparse.h"
#include "eventservice/log/log_client.h"
#include "eventservice/log/log_server.h"

using namespace std;

YamlValue::YamlValue(const YAML::Node &node):
m_node(node)
{

}

const YamlValue YamlValue::getSub(const char *key) const
{
    
    if(!m_node || !m_node.IsMap())
    {     
        DLOG_ERROR(MOD_EB,"type error(not map). content:");
        return YAML::Node();
    }
   
    const YAML::Node& node = m_node[key];
   
    if(!node)
    {      
        DLOG_ERROR(MOD_EB,"Yaml get key fail. key:");
        return YamlValue(node);//YAML::Node();
    }
    DLOG_DEBUG(MOD_EB,"Yaml get key success. key:");
    return YamlValue(node);
}

const YamlValue YamlValue::getSeq(const char *key) const
{
    
    if(!m_node || !m_node.IsSequence())
    {
        DLOG_ERROR(MOD_EB,"type error(not map). content:");
        return YAML::Node();
    }
    const YAML::Node& node = m_node[key];
    if(!node)
    {     
        DLOG_ERROR(MOD_EB,"Yaml get key fail. key:");
        return YamlValue(node);//YAML::Node();
    }
    DLOG_DEBUG(MOD_EB,"Yaml get key success. key:");
    return YamlValue(node);
}


std::string YamlValue::getString(const char *key) const
{
    if(!m_node || !m_node.IsMap())
    {
        DLOG_ERROR(MOD_EB,"type error(not map). content:");
        return "";
    }
    const YAML::Node& node = m_node[key];
    if(!node)
    {
        DLOG_ERROR(MOD_EB,"Yaml get key fail. key:");
        return "";
    }
    return node.as<std::string>();
}

float YamlValue::getFloat(const char* key, float defaultValue) const
{
    if(!m_node || !m_node.IsMap())
    {
        DLOG_ERROR(MOD_EB,"type error(not map). content:");
        return defaultValue;
    }
    const YAML::Node& node = m_node[key];
    try {
        if(!node)
        {
          DLOG_ERROR(MOD_EB,"Yaml get key fail. key:");
        } else {
            return node.as<float>();
        }
    } catch (YAML::TypedBadConversion<float>& e) {
      DLOG_ERROR(MOD_EB,"Yaml convert fail key:%s error:",e.msg);
    }

    return defaultValue;
}

int YamlValue::getInt(const char* key, int defaultValue) const
{
    if(!m_node || !m_node.IsMap())
    {
        DLOG_ERROR(MOD_EB,"type error(not map). content:");
        return defaultValue;
    }
    const YAML::Node& node = m_node[key];
    try {
        if(!node)
        {
          DLOG_ERROR(MOD_EB,"Yaml key not found");
        } else {
            return node.as<int>();
        }
    } catch (YAML::TypedBadConversion<int>& e) {
        DLOG_ERROR(MOD_EB,"Yaml convert fail key %s. error:",e.msg);
    }
    return defaultValue;
}

YamlConfig::YamlConfig():
m_yamlValue(nullptr)
{

}
YamlConfig::YamlConfig(const YamlConfig &config)
{
    m_root = config.m_root;
    m_yamlValue = new YamlValue(m_root);
}
YamlConfig& YamlConfig::operator=(const YamlConfig &config)
{
    m_root = config.m_root;
    if(m_yamlValue)
    {
        delete m_yamlValue;
    }
    m_yamlValue = new YamlValue(m_root);

    return *this;
}
YamlConfig::~YamlConfig()
{
    if(m_yamlValue)
    {
        delete m_yamlValue;
    }
}

bool YamlConfig::init(const char *file)
{
    try {
        m_root = YAML::LoadFile(file);
        m_yamlValue = new YamlValue(m_root);
        //初始化一个YamlValue m_root 传到YamlValue里面的root
        return true;
    } catch (YAML::ParserException& e) {
      DLOG_ERROR(MOD_EB,"Yaml LoadFile fail. error:%s",e.msg);
    } catch (YAML::BadFile& e) {
      DLOG_ERROR(MOD_EB,"Yaml LoadFile fail. error:%s",e.msg);
    }

    return false;
}

void YamlConfig::printAll()
{
    //debug
//    YAML::Emitter out;
//    out << root;
    std::string out = YAML::Dump(m_root);
 //   LOG4_DEBUG_STR(out.c_str());
}


// int parsing_yaml(YAML::Node &config)
// {
    
//     try{
//         config = YAML::LoadFile(YAMLPATH);
//     } catch(YAML::BadFile &e) {
//         cout<<"read error! "<<e.msg<<endl; 
//          return -1;    
//     } catch (YAML::ParserException& e) {
//         cout<<"read error! "<<e.msg<<endl;  
//          return -1;  
//     }
//     ofstream fout(YAMLPATH);
//     cout << "Node type " << config.Type() << endl; 
//     cout << "device_type:" << config["machine"]["device_type"].as<string>() << endl;
//     cout << "device_sub_type:"  << config["machine"]["device_sub_type"].as<string>() << endl;
//     cout << "version:"  << config["machine"]["version"].as<string>() << endl;
//     cout << "version_id:"  << config["machine"]["version_id"].as<int>() << endl;

//     //关闭配置文件
//     // fout << config;
//     // fout.close();
//     return 1;
// }