#ifndef  VOICE_MODULE_H
#define  VOICE_MODULE_H
#include <functional>
#include <mutex>
#include <queue>
#include <future>
#include <chrono>
#include "peripherals_server/base/base_module.h"
#include "voice_play_config.h"
namespace peripherals{

class VoiceModule {

public:
  VoiceModule();
  ~VoiceModule();
  VoiceModule(VoiceModule const&);
  VoiceModule& operator=(VoiceModule const&);
  static std::mutex m_instMutex;//定义单例模式的访问互斥锁
  static VoiceModule* getVoiceModule();
  void VoicePlaytts(int isoffline,const char *tts);
  void VoicePlayService();
private:
  static VoiceModule* p_Instance; //单例模式的静态类指针
  std::thread m_thPlay; //线程
  std::mutex m_voiceMutex;
  std::queue<std::string> m_taskQ;
  std::condition_variable m_voiceCond;
  bool m_srvRunning;
};

}
#endif // ! 
