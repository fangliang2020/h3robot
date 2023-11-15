#ifndef YAMLPARSE_H
#define YAMLPARSE_H
#include <yaml-cpp/yaml.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <iostream>
#include <fstream>


class YamlValue
{
public:
    YamlValue(const YAML::Node& node);
    /**
     * 如果有子yaml对象，可先调用此接口
     * @param key
     * @return yaml的对象
     */
    const YamlValue getSub(const char* key) const;
    /**
     * 如果有seq对象，可先调用此接口
     * @param key
     * @return yaml的对象
     */
    const YamlValue getSeq(const char* key) const;
    /**
     * 读取字符串类型的value，如果key不存在，返回""空字符串
     * @param key
     * @return
     */
    std::string getString(const char* key) const;

    /**
     * 读取float类型value
     * @param key
     * @param defaultValue 如果key不存在或者类型错误，返回此值作为默认
     * @return value
     */
    float getFloat(const char* key, float defaultValue = 0.0f) const;

    /**
     * 读取int类型value
     * @param key
     * @param defaultValue 如果key不存在或者类型错误，返回此值作为默认
     * @return value
     */
    int getInt(const char* key, int defaultValue = 0) const;
    friend std::ostream& operator<<(std::ostream& out,const YamlValue& value);
    explicit operator bool()
    {
        return !!m_node;
    }

public:
    const YAML::Node  m_node;     
};
inline std::ostream& operator<< (std::ostream& out, const YamlValue& value)
{
    if(!value.m_node)
    {
        return out << "";
    }
    return out << value.m_node;
}

class YamlConfig
{
public:
    YamlConfig();
    YamlConfig(const YamlConfig& config);
    YamlConfig& operator= (const YamlConfig& config);
    ~YamlConfig();

    bool init(const char* file);

    const YamlValue& root()
    {
        return *m_yamlValue;
    }

    void printAll();

public:
    YAML::Node m_root;
    YamlValue* m_yamlValue;

};


#ifdef __cplusplus  
extern "C"{   
#endif
   
int parsing_yaml(YAML::Node &config);

#ifdef __cplusplus  
}  
#endif


#endif

