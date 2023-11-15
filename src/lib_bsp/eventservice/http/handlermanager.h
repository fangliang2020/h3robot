#ifndef DEVICEGROUP_HTTP_HANDLER_MANAGER_H_
#define DEVICEGROUP_HTTP_HANDLER_MANAGER_H_

#include <map>

#include "eventservice/net/asynchttpsocket.h"

#include "eventservice/log/log_client.h"

namespace chml {

class HttpHandler : public boost::noncopyable {
 public:
  typedef boost::shared_ptr<HttpHandler> Ptr;
  explicit HttpHandler(EventService::Ptr event_service)
    : event_service_(event_service) {
  }
  virtual ~HttpHandler() {
    DLOG_INFO(MOD_EB, "Delete HttpHandler");
  }
  virtual bool HandleRequest(AsyncHttpSocket::Ptr connect,
                             HttpReqMessage& request) = 0;
 private:
  EventService::Ptr event_service_;
};

class HandlerManager : public boost::noncopyable,
  public boost::enable_shared_from_this<HandlerManager> {
 public:
  typedef boost::shared_ptr<HandlerManager> Ptr;
  HandlerManager();
  virtual ~HandlerManager();
  bool SetDefualtHandler(HttpHandler::Ptr handler);
  bool AddRequestHandler(const std::string path, HttpHandler::Ptr handler);
  void OnNewRequest(AsyncHttpSocket::Ptr connect, HttpReqMessage& request);
 private:
  HttpHandler::Ptr                        defualt_handler_;
  typedef std::map<std::string, HttpHandler::Ptr> Handlers;
  Handlers                                handlers_;
};
}  // namespace chml

#endif  // DEVICEGROUP_HTTP_HANDLER_MANAGER_H_
