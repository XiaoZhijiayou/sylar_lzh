#include "../sylar/config.h"
#include "../sylar/log.h"
#include "yaml-cpp/yaml.h"

sylar::ConfigVar<int>::ptr g_int_value_config =
    sylar::Config::Lookup("system.port",(int)8080,"system port");

void print_yaml(const YAML::Node node,int level){
    if(node.IsScalar()){
      SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4,' ')
                                       << node.Scalar() << "-" << node.Tag();
    }else if(node.IsNull()){
      SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4,' ')
          << "NULL - " <<node.Type() << "-" << level;
    }else if(node.IsMap()){
        for(auto it = node.begin();
                it != node.end(); ++it){
          SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4,' ')
              << it->first << " - " << it->second.Type() << " - " << level;
          print_yaml(it->second,level + 1);
        }
    }else if(node.IsSequence()){
      for(size_t i = 0; i < node.size(); ++i){
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4,' ')
            << i << " - " << node[i].Type() << " - " << level;
        print_yaml(node[i],level + 1);
      }
    }
}

void test_yaml(){
    YAML::Node root = YAML::LoadFile("/home/li/project/wbeserver_all/sylar/bin/conf/log.yml");
    print_yaml(root,0);
    //SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << root;

}
int main(int argc, char** argv){
  SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_value_config->getValue();
  SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << g_int_value_config->toString();
  test_yaml();
  return 0;
}