#include "voice_module.h"
#include "peripherals_server/base/jkey_common.h"
namespace peripherals{
   
VoiceModule* VoiceModule::p_Instance=nullptr;
std::mutex VoiceModule::m_instMutex;

VoiceModule::VoiceModule()
{
  DLOG_DEBUG(MOD_EB,"Voice_Module\n");
  m_srvRunning=true;
  m_thPlay = std::thread(&VoiceModule::VoicePlayService,this);
}

VoiceModule* VoiceModule::getVoiceModule()
{
  if(p_Instance==nullptr)
  {
    m_instMutex.lock();
    if(p_Instance==nullptr)
    {
      p_Instance = new VoiceModule();
    }
    m_instMutex.unlock();
  }
  return p_Instance;
}

void VoiceModule::VoicePlaytts(int isoffline,const char *tts)
{
  std::string cmd = "aplay -D plughw:1,0 /oem/voice_play/";
  cmd += std::string(tts) + ".wav";
  DLOG_INFO(MOD_EB,"%s\n",cmd.c_str());
  std::lock_guard<std::mutex> lk(m_voiceMutex);
  m_taskQ.push(cmd);
  m_voiceCond.notify_all();
}

void VoiceModule::VoicePlayService()
{
  while(m_srvRunning)
  {
    std::unique_lock<std::mutex> lk(m_voiceMutex);
    m_voiceCond.wait(lk, [this](){
        return !m_taskQ.empty() || !m_srvRunning;
    });
    if(!m_taskQ.empty())
    {
        std::string cmd = m_taskQ.front();
        m_taskQ.pop();
        lk.unlock();
        system(cmd.c_str());
    }
    else
    {
      lk.unlock();
    }
  }
}

VoiceModule::~VoiceModule()
{
   std::unique_lock<std::mutex> lk(m_voiceMutex);
   m_srvRunning = false;
   lk.unlock();
   m_voiceCond.notify_all();
   m_thPlay.join();
}

}