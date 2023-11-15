#ifndef EVENTSERVICES_BASE_PING_H__
#define EVENTSERVICES_BASE_PING_H__

namespace chml {

class Ping {
 public:
  Ping();
  ~Ping();

  // 成功为true，失败为false
  static bool ping(char *dev, char *ip);

 private:
};

} // namespace chml

#endif // EVENTSERVICES_BASE_PING_H__

