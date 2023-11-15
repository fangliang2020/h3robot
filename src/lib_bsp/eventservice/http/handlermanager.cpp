#include "eventservice/http/handlermanager.h"
#include "eventservice/log/log_client.h"
#include <iostream>

namespace chml {

HandlerManager::HandlerManager() {
}

HandlerManager::~HandlerManager() {
  DLOG_INFO(MOD_EB, "Delete HandlerManager");
}

bool HandlerManager::SetDefualtHandler(HttpHandler::Ptr handler) {
  if (defualt_handler_) {
    DLOG_INFO(MOD_EB, "It was set the defualt handler");
    return false;
  }
  defualt_handler_ = handler;
  return true;
}

bool HandlerManager::AddRequestHandler(const std::string path,
                                       HttpHandler::Ptr handler) {
  Handlers::iterator iter = handlers_.find(path);
  if (iter != handlers_.end()) {
    DLOG_INFO(MOD_EB, "It has beend a hand");
    return false;
  }
  handlers_.insert(
    std::map<std::string, HttpHandler::Ptr>::value_type(path, handler));
  return true;
}

void HandlerManager::OnNewRequest(AsyncHttpSocket::Ptr connect,
                                  HttpReqMessage& request) {
  DLOG_INFO(MOD_EB, "%s : %s", request.method.c_str(), request.req_url.c_str());
  Handlers::iterator iter = handlers_.find(request.req_url);
  if (iter != handlers_.end()) {
    iter->second->HandleRequest(connect, request);
  } else {
    defualt_handler_->HandleRequest(connect, request);
  }
}
}  // namespace chml
