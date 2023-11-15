#ifndef HTTP_ACCESS_CURL_MANAGER_H_
#define HTTP_ACCESS_CURL_MANAGER_H_

#include "eventservice/net/eventservice.h"
#include "eventservice/log/log_client.h"
#include "eventservice/event/messagehandler.h"
#include "curl/curl.h"

enum HttpMethods {
  HTTP_REQ_GET = 1,
  HTTP_REQ_POST = 2,
  HTTP_REQ_HEAD = 3,
  HTTP_REQ_PUT = 4,
  HTTP_REQ_DELETE = 5,
  HTTP_REQ_OPTIONS = 6,
  HTTP_REQ_TRACE = 7,
  HTTP_REQ_CONNECT = 8,
  HTTP_REQ_PATCH = 9
};

enum FormType {
  FORM_COPYCONTENTS = 1,
  FORM_FILE = 2,
};

std::string GetMethodString(HttpMethods method);

class CurlRequest : public boost::noncopyable,
  public boost::enable_shared_from_this<CurlRequest> {
  friend class CurlManager;
 public:
  typedef boost::shared_ptr<CurlRequest> Ptr;

  sigslot::signal3<char *, size_t, size_t *> SignalOnWrite;

 protected:
  CurlRequest();
 public:
  virtual ~CurlRequest();

 public:
  CURL *easy_handle() {
    return easy_handle_;
  }

  bool SetUrl(const std::string &url);

  bool SetSSL(const std::string &ssl_path);

  std::string GetUrl() const {
    return url_;
  }

  bool AddHeader(const std::string &key, const std::string &value);
  bool AddFormBuffer(const std::string &name, std::string filen_path,
                         const char* buff, int32 buf_len);
  bool AddFile(const std::string &name, const std::string &path);
  bool AddFile(const std::string &name, chml::MemBuffer::Ptr buff, const std::string& filename = "", const std::string& contenttype = "");
  bool AddForm(const std::string &name, const std::string &content, const std::string& filename = "", const std::string& contenttype = "");
  bool SetTimeout(long timeout);
  bool SetConnectTimeout(long timeout);
  bool SetRetryTimes(int retry_times);
  virtual bool OpenFile();
  virtual void CloseFile();

  bool SetBodyData(const std::string &data);
  bool SetMethod(HttpMethods method);

  void SetUserType(std::string user_type) {
    user_type_ = user_type;
  }

  void SetUserData(chml::MessageData::Ptr data) {
    user_data_ = data;
  }

  const std::string GetUserType() const {
    return user_type_;
  }

  std::string GetFilePath() const {
    return file_path_;
  }

  chml::MessageData::Ptr GetUserData() const {
    return user_data_;
  }

  const std::string GetResponseData() const {
    return resp_data_;
  }

  // 同步接口
  bool Execute();

 private:
  static size_t WriteCallback(char *ptr,
                              size_t size,
                              size_t nmemb,
                              void *userdata);

  static size_t ReadCallback(char *ptr, size_t size, size_t nmemb, void *userdata);

 protected:
  size_t OnWrite(char *ptr, size_t size, size_t nmemb);
  virtual size_t OnRead(char *ptr, size_t size, size_t nmemb);
  bool GetReady();

 protected:
  std::string url_;
  CURL *easy_handle_;
  struct curl_slist *header_list_;
  struct curl_httppost *post_;
  struct curl_httppost *last_;
  std::string body_data_;
  std::string resp_data_;
  std::string user_type_;
  chml::MessageData::Ptr user_data_;
  struct timeval begin_tv_;
  chml::MemBuffer::Ptr send_buffer_;
  size_t send_pos_;
  int retry_times_;
  std::string file_path_;
};


class CurlManager : public boost::noncopyable,
  public boost::enable_shared_from_this<CurlManager>,
  public sigslot::has_slots<>,
  public chml::MessageHandler {
 public:
  typedef boost::shared_ptr<CurlManager> Ptr;

  CurlManager(chml::EventService::Ptr es);
  virtual ~CurlManager();

  sigslot::signal1<CurlRequest::Ptr> SignalDone;
  sigslot::signal2<int, CurlRequest::Ptr> SignalError;

 public:
  bool Start();

  CurlRequest::Ptr CreateRequest(HttpMethods method = HTTP_REQ_GET);

  bool AddRequest(CurlRequest::Ptr request);

  size_t RequestCount() {
    return requests_.size();
  }

 protected:
  //消息处理
  virtual void OnMessage(chml::Message *msg);

  void CheckMultiInfo();
  void OnMultiInsert(CurlRequest::Ptr request);
  void OnMultiSelect();
 private:
  chml::EventService::Ptr es_;
  CURLM *multi_handle_;
  std::list<CurlRequest::Ptr> requests_;
  int running_handles_;
  bool is_stop_;
  bool is_pause_;
};

#endif
