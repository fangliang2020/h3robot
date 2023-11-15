#include "curl_manager.h"

#define ON_CURL_MSG_MULTI_SELECT    0x3001
#define ON_CURL_MSG_MULTI_INSERT    0x3002
#define CURL_CONNECT_COUNT_MAX      8

std::string GetMethodString(HttpMethods method) {
  switch (method) {
  case HTTP_REQ_GET:
    return "GET";
  case HTTP_REQ_POST:
    return "POST";
  case HTTP_REQ_HEAD:
    return "HEAD";
  case HTTP_REQ_PUT:
    return "PUT";
  case HTTP_REQ_DELETE:
    return "DELETE";
  case HTTP_REQ_OPTIONS:
    return "OPTIONS";
  case HTTP_REQ_TRACE:
    return "TRACE";
  case HTTP_REQ_CONNECT:
    return "CONNECT";
  case HTTP_REQ_PATCH:
    return "PATCH";
  default:
    return "GET";
  }
}

CurlRequest::CurlRequest()
  : easy_handle_(nullptr), header_list_(nullptr), post_(nullptr), last_(nullptr), send_pos_(0), retry_times_(0) {
  easy_handle_ = curl_easy_init();
  printf("Curl Handle New:0x%x.\n", easy_handle_);
}

CurlRequest::~CurlRequest() {
  if (header_list_) {
    curl_slist_free_all(header_list_);
    header_list_ = nullptr;
  }
  if (post_) {
    curl_formfree(post_);
    post_ = nullptr;
  }
  printf("Curl Handle Del:0x%x.\n", easy_handle_);
  curl_easy_cleanup(easy_handle_);
}

bool CurlRequest::SetUrl(const std::string &url) {
  url_ = url;

  CURLcode ret;
  ret = curl_easy_setopt(easy_handle_, CURLOPT_URL, url.c_str());
  if (ret != CURLE_OK) {
    DLOG_ERROR(MOD_EB, "Set CURLOPT_URL error: %d!", ret);
    return false;
  }
  return true;
}

bool CurlRequest::SetSSL(const std::string &ssl_path) {
  CURLcode ret;
  if (ssl_path.size() > 0) {
    ret = curl_easy_setopt(easy_handle_, CURLOPT_SSLCERT, ssl_path.c_str());
    if (ret != CURLE_OK) {
      DLOG_ERROR(MOD_EB, "Set CURLOPT_SSLCERT error: %d!", ret);
      return false;
    }
  }

  return true;
}

bool CurlRequest::AddHeader(const std::string &key, const std::string &value) {
  header_list_ = curl_slist_append(header_list_, (key + ": " + value).c_str());
  if (header_list_ == NULL) {
    DLOG_ERROR(MOD_EB, "Call curl_slist_append error!");
    return false;
  }
  return true;
}

bool CurlRequest::SetTimeout(long timeout) {
  CURLcode ret;
  ret = curl_easy_setopt(easy_handle_, CURLOPT_TIMEOUT, timeout);
  if (ret != CURLE_OK) {
    DLOG_ERROR(MOD_EB, "Set CURLOPT_TIMEOUT error: %d!", ret);
    return false;
  }
  return true;
}

bool CurlRequest::SetRetryTimes(int retry_times) {
  retry_times_ = retry_times;
  return true;
}

bool CurlRequest::SetConnectTimeout(long timeout) {
  CURLcode ret;
  ret = curl_easy_setopt(easy_handle_, CURLOPT_CONNECTTIMEOUT, timeout);
  if (ret != CURLE_OK) {
    DLOG_ERROR(MOD_EB, "Set CURLOPT_CONNECTTIMEOUT error: %d!", ret);
    return false;
  }
  return true;
}

bool CurlRequest::SetBodyData(const std::string &data) {
  body_data_ = data;

  CURLcode ret;
  ret = curl_easy_setopt(easy_handle_, CURLOPT_POSTFIELDS,
                         body_data_.c_str());
  if (ret != CURLE_OK) {
    DLOG_ERROR(MOD_EB, "Set CURLOPT_POSTFIELDS error: %d!", ret);
    return false;
  }
  ret = curl_easy_setopt(easy_handle_, CURLOPT_POSTFIELDSIZE,
                         body_data_.size());
  if (ret != CURLE_OK) {
    DLOG_ERROR(MOD_EB, "Set CURLOPT_POSTFIELDSIZE error: %d!", ret);
    return false;
  }
  return true;
}

bool CurlRequest::AddFile(const std::string &name, const std::string &path) {
  CURLFORMcode ret;
  ret = curl_formadd(&post_, &last_,
                     CURLFORM_COPYNAME, name.c_str(),
                     CURLFORM_FILE, path.c_str(),
                     CURLFORM_END);
  if (ret != CURL_FORMADD_OK) {
    DLOG_ERROR(MOD_EB, "Call curl_formadd error: %d!", ret);
    return false;
  }
  return true;
}

// 这个只支持一条信令单个文件
bool CurlRequest::AddFile(const std::string &name, chml::MemBuffer::Ptr buff,
                          const std::string& filename/* = ""*/,
                          const std::string& contenttype/* = ""*/) {
  CURLFORMcode ret;
  send_buffer_ = buff;
  if (!filename.empty() && !contenttype.empty()) {
    ret = curl_formadd(&post_, &last_,
                       CURLFORM_COPYNAME, name.c_str(),
                       CURLFORM_STREAM, this,
                       CURLFORM_CONTENTSLENGTH, (long)buff->size(),
                       CURLFORM_FILENAME, filename.c_str(),
                       CURLFORM_CONTENTTYPE, contenttype.c_str(),
                       CURLFORM_END);
  } else if (filename.empty() && !contenttype.empty()) {
    ret = curl_formadd(&post_, &last_,
                       CURLFORM_COPYNAME, name.c_str(),
                       CURLFORM_STREAM, this,
                       CURLFORM_CONTENTSLENGTH, (long)buff->size(),
                       CURLFORM_CONTENTTYPE, contenttype.c_str(),
                       CURLFORM_END);
  } else if (!filename.empty() && contenttype.empty()) {
    ret = curl_formadd(&post_, &last_,
                       CURLFORM_COPYNAME, name.c_str(),
                       CURLFORM_STREAM, this,
                       CURLFORM_CONTENTSLENGTH, (long)buff->size(),
                       CURLFORM_FILENAME, filename.c_str(),
                       CURLFORM_END);
  } else {
    ret = curl_formadd(&post_, &last_,
                       CURLFORM_COPYNAME, name.c_str(),
                       CURLFORM_STREAM, this,
                       CURLFORM_CONTENTSLENGTH, (long)buff->size(),
                       CURLFORM_END);
  }
  if (ret != CURL_FORMADD_OK) {
    DLOG_ERROR(MOD_EB, "Call curl_formadd error: %d!", ret);
    return false;
  }
  return true;
}

bool CurlRequest::AddFormBuffer(const std::string &name, std::string filen_path,
                                const char* buff, int32 buf_len) {
  if (name.empty() || NULL == buff || buf_len <= 0) {
    return false;
  }

  int32 pos = filen_path.find("pic_zone");
  if (std::string::npos != pos) {
    file_path_ = filen_path.substr(pos, 12);
  }
  pos = filen_path.rfind("/");
  if (std::string::npos != pos) {
    filen_path = filen_path.substr(filen_path.rfind("/") + 1, filen_path.size());
  }
  CURLFORMcode ret = curl_formadd(&post_, &last_,
                                  CURLFORM_COPYNAME, name.c_str(),
                                  CURLFORM_FILENAME, filen_path.c_str(),
                                  CURLFORM_BUFFERPTR, buff,
                                  CURLFORM_BUFFERLENGTH, buf_len,
                                  CURLFORM_END);

  DLOG_DEBUG(MOD_EB, "Add From Name:%s, Buf:0x%x, len:%d, ret:%d.\n",
             name.c_str(), buff, buf_len, ret);
  return (CURL_FORMADD_OK == ret ? true : false);
}

bool CurlRequest::AddForm(const std::string &name, const std::string &content, const std::string& filename/* = ""*/, const std::string& contenttype/* = ""*/) {
  CURLFORMcode ret;
  if (!filename.empty() && !contenttype.empty()) {
    ret = curl_formadd(&post_, &last_, CURLFORM_COPYNAME, name.c_str(),
                       CURLFORM_COPYCONTENTS, content.c_str(),
                       CURLFORM_CONTENTSLENGTH, content.length(),
                       CURLFORM_FILENAME, filename.c_str(),
                       CURLFORM_CONTENTTYPE, contenttype.c_str(),
                       CURLFORM_END);
  } else if (filename.empty() && !contenttype.empty()) {
    ret = curl_formadd(&post_, &last_, CURLFORM_COPYNAME, name.c_str(),
                       CURLFORM_COPYCONTENTS, content.c_str(),
                       CURLFORM_CONTENTSLENGTH, content.length(),
                       CURLFORM_CONTENTTYPE, contenttype.c_str(),
                       CURLFORM_END);
  } else if (!filename.empty() && contenttype.empty()) {
    ret = curl_formadd(&post_, &last_, CURLFORM_COPYNAME, name.c_str(),
                       CURLFORM_COPYCONTENTS, content.c_str(),
                       CURLFORM_CONTENTSLENGTH, content.length(),
                       CURLFORM_FILENAME, filename.c_str(),
                       CURLFORM_END);
  } else {
    ret = curl_formadd(&post_, &last_, CURLFORM_COPYNAME, name.c_str(),
                       CURLFORM_COPYCONTENTS, content.c_str(),
                       CURLFORM_CONTENTSLENGTH, content.length(),
                       CURLFORM_END);
  }
  if (ret != CURL_FORMADD_OK) {
    DLOG_ERROR(MOD_EB, "Call curl_formadd error: %d!", ret);
    return false;
  }
  return true;
}

bool CurlRequest::SetMethod(HttpMethods method) {
  CURLcode ret;
  ret = curl_easy_setopt(easy_handle_, CURLOPT_CUSTOMREQUEST,
                         GetMethodString(method).c_str());
  if (ret != CURLE_OK) {
    DLOG_ERROR(MOD_EB, "Set CURLOPT_CUSTOMREQUEST error: %d!", ret);
    return false;
  }
  return true;
}

bool CurlRequest::Execute() {
  if (!GetReady()) return false;

  CURLcode code = curl_easy_perform(easy_handle_);
  if (code == CURLE_OK) {
    long resp_code;
    code = curl_easy_getinfo(easy_handle_, CURLINFO_RESPONSE_CODE, &resp_code);
    if (code == CURLE_OK && resp_code == 200) {
      return true;
    }
  }
  return false;
}

size_t CurlRequest::WriteCallback(char *ptr,
                                  size_t size,
                                  size_t nmemb,
                                  void *userdata) {
  CurlRequest::Ptr live_this = ((CurlRequest *) userdata)->shared_from_this();
  return live_this->OnWrite(ptr, size, nmemb);
}

size_t CurlRequest::ReadCallback(char *ptr, size_t size, size_t nmemb, void *userdata) {
  CurlRequest::Ptr live_this = ((CurlRequest *) userdata)->shared_from_this();
  return live_this->OnRead(ptr, size, nmemb);
}

size_t CurlRequest::OnWrite(char *ptr, size_t size, size_t nmemb) {
  if (NULL == ptr) {
    return -1;
  }
  size_t realsize = size * nmemb;
  if (SignalOnWrite.is_empty()) {
    resp_data_.append(ptr, realsize);
  } else {
    SignalOnWrite(ptr, realsize, &realsize);
  }
  return realsize;
}

size_t CurlRequest::OnRead(char *ptr, size_t size, size_t nmemb) {
  size_t realsize = size * nmemb;
  size_t read = send_buffer_->CopyBytes(ptr, send_pos_, realsize);
  send_pos_ += read;

  printf("---CurlRequest::OnRead realsize:%d, read:%d, send_pos_:%d.\n",
         realsize, read, send_pos_);
  return read;
}

bool CurlRequest::GetReady() {
  CURLcode ret;

  curl_easy_setopt(easy_handle_, CURLOPT_VERBOSE, 1L); // 打印请求连接过程和返回http数据

  ret = curl_easy_setopt(easy_handle_, CURLOPT_HTTPHEADER, header_list_);
  if (ret != CURLE_OK) {
    DLOG_ERROR(MOD_EB, "Set CURLOPT_HTTPHEADER error: %d!", ret);
    return false;
  }

  if (post_ != NULL) {
    ret = curl_easy_setopt(easy_handle_, CURLOPT_HTTPPOST, post_);
    if (ret != CURLE_OK) {
      DLOG_ERROR(MOD_EB, "Set CURLOPT_HTTPPOST error: %d!", ret);
      return false;
    }
  }

  ret = curl_easy_setopt(easy_handle_, CURLOPT_WRITEDATA, this);
  if (ret != CURLE_OK) {
    DLOG_ERROR(MOD_EB, "Set CURLOPT_WRITEDATA error: %d!", ret);
    return false;
  }

  ret = curl_easy_setopt(easy_handle_, CURLOPT_WRITEFUNCTION, WriteCallback);
  if (ret != CURLE_OK) {
    DLOG_ERROR(MOD_EB, "Set CURLOPT_WRITEFUNCTION error: %d!", ret);
    return false;
  }

  ret = curl_easy_setopt(easy_handle_, CURLOPT_READDATA, this);
  if (ret != CURLE_OK) {
    DLOG_ERROR(MOD_EB, "Set CURLOPT_READDATA error: %d!", ret);
    return false;
  }

  ret = curl_easy_setopt(easy_handle_, CURLOPT_READFUNCTION, ReadCallback);
  if (ret != CURLE_OK) {
    DLOG_ERROR(MOD_EB, "Set CURLOPT_READFUNCTION error: %d!", ret);
    return false;
  }

  ret = curl_easy_setopt(easy_handle_, CURLOPT_FOLLOWLOCATION, 1L);
  if (ret != CURLE_OK) {
    DLOG_ERROR(MOD_EB, "Set CURLOPT_FOLLOWLOCATION error: %d!", ret);
    return false;
  }

  ret = curl_easy_setopt(easy_handle_, CURLOPT_PRIVATE, this);
  if (ret != CURLE_OK) {
    DLOG_ERROR(MOD_EB, "Set CURLOPT_PRIVATE error: %d!", ret);
    return false;
  }

  ret = curl_easy_setopt(easy_handle_, CURLOPT_SSL_VERIFYPEER, 0L);
  if (ret != CURLE_OK) {
    DLOG_ERROR(MOD_EB, "Set CURLOPT_SSL_VERIFYPEER error: %d!", ret);
    return false;
  }

  ret = curl_easy_setopt(easy_handle_, CURLOPT_SSL_VERIFYHOST, 0L);
  if (ret != CURLE_OK) {
    DLOG_ERROR(MOD_EB, "Set CURLOPT_SSL_VERIFYHOST error: %d!", ret);
    return false;
  }

  return true;
}


bool CurlRequest::OpenFile() {
  resp_data_ = "";
  return true;
}
void CurlRequest::CloseFile() {

}

CurlManager::CurlManager(chml::EventService::Ptr es)
  : es_(es), is_stop_(false), is_pause_(true) {
}

CurlManager::~CurlManager() {
  if (multi_handle_) {
    curl_multi_cleanup(multi_handle_);
    multi_handle_ = NULL;
  }
}

bool CurlManager::Start() {
  multi_handle_ = curl_multi_init();
  if (NULL == multi_handle_) {
    DLOG_ERROR(MOD_REC, "Call curl_multi_init error!");
    return false;
  }

  return true;
}

CurlRequest::Ptr CurlManager::CreateRequest(HttpMethods method /*= HTTP_REQ_GET*/) {
  CurlRequest::Ptr request(new CurlRequest);
  request->SetMethod(method);
  return request;
}

bool CurlManager::AddRequest(CurlRequest::Ptr request) {
  chml::TypedMessageData<CurlRequest::Ptr>::Ptr param =
    chml::TypedMessageData<CurlRequest::Ptr>::Ptr(
      new chml::TypedMessageData<CurlRequest::Ptr>(request));

  es_->Post(this, ON_CURL_MSG_MULTI_INSERT, param);
  return true;
}

void CurlManager::OnMessage(chml::Message* msg) {
  if (ON_CURL_MSG_MULTI_SELECT == msg->message_id) {
    OnMultiSelect();
    return;
  }

  // 推送插入数据
  if (ON_CURL_MSG_MULTI_INSERT == msg->message_id) {
    chml::TypedMessageData<CurlRequest::Ptr>::Ptr pdata =
      boost::static_pointer_cast<chml::TypedMessageData<CurlRequest::Ptr>>(msg->pdata);
    OnMultiInsert(pdata->data());
    return;
  }

  return;
}

void CurlManager::OnMultiInsert(CurlRequest::Ptr request) {
  if (requests_.size() > CURL_CONNECT_COUNT_MAX) {
    DLOG_ERROR(MOD_BUS, "Max Curl Count:%d, Over:%d.", requests_.size(), CURL_CONNECT_COUNT_MAX);
    SignalError(CURLE_FAILED_INIT, request);
    return;
  }

  if (!request->GetReady()) {
    DLOG_ERROR(MOD_BUS, "Ready Curl Req Failed.");
    SignalError(CURLE_FAILED_INIT, request);
    return;
  }

  CURL *easy_handle = request->easy_handle();
  CURLMcode ret = curl_multi_add_handle(multi_handle_, easy_handle);
  if (CURLM_OK != ret) {
    DLOG_ERROR(MOD_BUS, "Add Handle Failed ret %d.", ret);
    SignalError(CURLE_FAILED_INIT, request);
    return;
  }

  requests_.push_back(request);
  DLOG_DEBUG(MOD_BUS, "Curl Handle Add:0x%x Count:%d.", easy_handle, requests_.size());

  curl_multi_perform(multi_handle_, &running_handles_);
  OnMultiSelect();
  return;
}

void CurlManager::OnMultiSelect() {
  if(!running_handles_) {
    CheckMultiInfo();
    // 此时应确保释放所有 request
    DLOG_ERROR(MOD_BUS, "MultiSelect Exit, Free All Requese Count:%d.", requests_.size());
    requests_.clear();
    return;
  }

  int32 rc;
  CURLMcode mc;

  int32  maxfd = -1;
  fd_set fdread;
  fd_set fdwrite;
  fd_set fdexcep;
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = 1000;

  FD_ZERO(&fdread);
  FD_ZERO(&fdwrite);
  FD_ZERO(&fdexcep);

  /* get file descriptors from the transfers */
  mc = curl_multi_fdset(multi_handle_, &fdread, &fdwrite, &fdexcep, &maxfd);
  if(mc != CURLM_OK) {
    DLOG_ERROR(MOD_BUS, "curl_multi_fdset() failed, code %d.\n", mc);
    return;
  }
  if(maxfd < 0) {
    // 当curl内部未处理完时, 等10ms再处理;
    curl_multi_perform(multi_handle_, &running_handles_);
    es_->Clear(this, ON_CURL_MSG_MULTI_SELECT);
    es_->PostDelayed(10, this, ON_CURL_MSG_MULTI_SELECT);
    return;
  }

  rc = select(maxfd + 1, &fdread, &fdwrite, &fdexcep, &timeout);
  if (-1 == rc) {
    DLOG_ERROR(MOD_BUS, "select error -1.");
  } else {
    /* timeout or readable/writable sockets */
    curl_multi_perform(multi_handle_, &running_handles_);
    es_->Clear(this, ON_CURL_MSG_MULTI_SELECT);
    es_->Post(this, ON_CURL_MSG_MULTI_SELECT);
  }

  CheckMultiInfo();
  return;
}

void CurlManager::CheckMultiInfo() {
  int32 msgq = 0;
  struct CURLMsg *curl_msg;
  do {
    curl_msg = curl_multi_info_read(multi_handle_, &msgq);
    if (curl_msg && (curl_msg->msg == CURLMSG_DONE)) {
      long resp_code;
      CurlRequest *req = NULL;
      CURL *easy_handle = curl_msg->easy_handle;
      CURLcode code = curl_msg->data.result;
      curl_easy_getinfo(easy_handle, CURLINFO_RESPONSE_CODE, &resp_code);
      curl_easy_getinfo(easy_handle, CURLINFO_PRIVATE, &req);
      curl_multi_remove_handle(multi_handle_, easy_handle);

      CurlRequest::Ptr request = req->shared_from_this();
      request->CloseFile();
      requests_.remove(request);
      DLOG_DEBUG(MOD_BUS, "Curl Handle Del:0x%x Count:%d.", easy_handle, requests_.size());

      printf("---Curl Handle Del:0x%x Count:%d.\n", easy_handle, requests_.size());
      if (CURLE_OK == code && (200 == resp_code || 204 == resp_code)) {
        SignalDone(request);
        continue;
      }

      if (request->retry_times_ > 0) {
        request->retry_times_--;
        DLOG_INFO(MOD_REC, "curl retry. retry_times: %d", request->retry_times_);
        LOG_POINT(161, 69,
                  "curl retry. retry_times: %d code: %d file: %s",
                  request->retry_times_, resp_code, request->GetUserType().c_str());
        request->OpenFile();
        AddRequest(request);
        continue;
      }

      SignalError(resp_code, request);
    }
  } while (curl_msg);
}
