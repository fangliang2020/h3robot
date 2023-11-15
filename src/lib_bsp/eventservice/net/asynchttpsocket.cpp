#include "asynchttpsocket.h"
#include "eventservice/log/log_client.h"

namespace chml {
////////////////////////////////////////////////////////////////////////////////
int CBHttpMessageBegin(http_parser *parser) {
  if (parser->data != NULL) {
    return ((AsyncHttpSocket *)parser->data)->OnHttpMessageBegin(parser);
  }
  return 0;
}
int CBHttpUrl(http_parser *parser, const char *at, std::size_t length) {
  if (parser->data != NULL) {
    return ((AsyncHttpSocket *)parser->data)->OnHttpUrl(parser, at, length);
  }
  return 0;
}

int CBHttpStatus(http_parser *parser, const char *at, std::size_t length) {
  if (parser->data != NULL) {
    return ((AsyncHttpSocket *)parser->data)->OnHttpStatus(parser, at, length);
  }
  return 0;
}

int CBHttpHeaderField(http_parser *parser, const char *at, std::size_t length) {
  if (parser->data != NULL) {
    return ((AsyncHttpSocket *)parser->data)->OnHttpHeaderField(
             parser, at, length);
  }
  return 0;
}

int CBHttpHeaderValue(http_parser *parser, const char *at, std::size_t length) {
  if (parser->data != NULL) {
    return ((AsyncHttpSocket *)parser->data)->OnHttpHeaderValue(
             parser, at, length);
  }
  return 0;
}

int CBHttpHeadersComplete(http_parser *parser) {
  if (parser->data != NULL) {
    return ((AsyncHttpSocket *)parser->data)->OnHttpHeadersComplete(parser);
  }
  return 0;
}
int CBHttpBody(http_parser *parser, const char *at, std::size_t length) {
  if (parser->data != NULL) {
    return ((AsyncHttpSocket *)parser->data)->OnHttpBody(parser, at, length);
  }
  return 0;
}
int CBHttpMessageComplete(http_parser *parser) {
  if (parser->data != NULL) {
    return ((AsyncHttpSocket *)parser->data)->OnHttpMessageComplete(parser);
  }
  return 0;
}

int CBHttpChunkHeader(http_parser *parser) {
  if (parser->data != NULL) {
    return ((AsyncHttpSocket *)parser->data)->OnHttpChunkHeader(parser);
  }
  return 0;
}

int CBHttpChunkComplete(http_parser *parser) {
  if (parser->data != NULL) {
    return ((AsyncHttpSocket *)parser->data)->OnHttpChunkComplete(parser);
  }
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
AsyncHttpSocket::AsyncHttpSocket(AsyncSocket::Ptr socket, SocketAddress &remote_addr)
  : async_socket_(socket) {
  recv_buffer_[0] = 0;
  async_socket_->SignalSocketErrorEvent.connect(this, &AsyncHttpSocket::OnAsyncSocketErrorEvent);
  async_socket_->SignalSocketReadEvent.connect(this, &AsyncHttpSocket::OnAsyncSocketReadEvent);
  async_socket_->SignalSocketWriteEvent.connect(this, &AsyncHttpSocket::OnAsyncSocketWriteEvent);

  //////////////////////////////////////////////////////////////////////////////
  // Init http settings
  http_settings_.on_message_begin     = chml::CBHttpMessageBegin;
  http_settings_.on_url               = chml::CBHttpUrl;
  http_settings_.on_status            = chml::CBHttpStatus;
  http_settings_.on_header_field      = chml::CBHttpHeaderField;
  http_settings_.on_header_value      = chml::CBHttpHeaderValue;
  http_settings_.on_headers_complete  = chml::CBHttpHeadersComplete;
  http_settings_.on_body              = chml::CBHttpBody;
  http_settings_.on_message_complete  = chml::CBHttpMessageComplete;
  http_settings_.on_chunk_header      = chml::CBHttpChunkHeader;
  http_settings_.on_chunk_complete    = chml::CBHttpChunkComplete;

  http_parser_init(&http_parser_, HTTP_REQUEST);
  http_parser_.data = (void *)this;

  recv_size_ = 0;
}

AsyncHttpSocket::~AsyncHttpSocket() {
  DLOG_WARNING(MOD_EB, "Close Http Sokcet");
  SignalClose(0, false);
}

void AsyncHttpSocket::Close() {
  SignalClose(0, false);
}

const SocketAddress AsyncHttpSocket::remote_addr() {
  BOOST_ASSERT(async_socket_);
  return async_socket_->remote_addr();
}

bool  AsyncHttpSocket::IsOpen() const {
  BOOST_ASSERT(async_socket_);
  return async_socket_->IsOpen();
}

bool  AsyncHttpSocket::IsClose() {
  BOOST_ASSERT(async_socket_);
  return async_socket_->IsClose();
}

//Socket::Ptr AsyncHttpSocket::SocketNumber() {
//  BOOST_ASSERT(async_socket_);
//  return async_socket_-;
//}

bool AsyncHttpSocket::ResponseWithStockReply(reply::status_type status) {
  if (!async_socket_ || async_socket_->IsClose()) {
    DLOG_ERROR(MOD_EB, "Socket is closed");
    return false;
  }
  reply reply_msg = reply_.stock_reply(status);
  return AsyncWriteRepMessage(reply_msg);
}

void AsyncHttpSocket::DumpHttpReqMessage(HttpReqMessage &http_req_message) {
  DLOG_INFO(MOD_EB, "%s", http_req_message.req_url.c_str());
  for (std::size_t i = 0; i < http_req_message.http_headers.size(); i++) {
    DLOG_INFO(MOD_EB, "name:%s, value:%s",
              http_req_message.http_headers[i].name.c_str(),
              http_req_message.http_headers[i].value.c_str());
  }
  DLOG_INFO(MOD_EB, "%s", http_req_message.body.c_str());
}
////////////////////////////////////////////////////////////////////////////////

bool AsyncHttpSocket::AsyncWritePacket(const char *data, uint32 size) {
  if (!async_socket_ || async_socket_->IsClose()) {
    DLOG_ERROR(MOD_EB, "Socket is closed");
    return false;
  }
  return async_socket_->AsyncWrite(data, size) == 0;
}

bool AsyncHttpSocket::AsyncWriteRepMessage(reply &reply) {
  if (!async_socket_ || async_socket_->IsClose()) {
    DLOG_ERROR(MOD_EB, "Socket is closed");
    return false;
  }
  std::string data = reply.to_string();
  //DLOG_INFO(MOD_EB, "data:%s", data);
  return AsyncWritePacket(data.c_str(), data.size());
}

bool AsyncHttpSocket::StartReadNextPacket() {
  if (!async_socket_ || async_socket_->IsClose()) {
    DLOG_ERROR(MOD_EB, "Socket is closed");
    return false;
  }
  // return true;
  return async_socket_->AsyncRead();
}

bool AsyncHttpSocket::AnalisysPacket(MemBuffer::Ptr buffer) {
  std::string packet = buffer->ToString();
  int res = http_parser_execute(&http_parser_,
                                &http_settings_,
                                packet.c_str(),
                                packet.size());
  DLOG_INFO(MOD_EB, "packet:%s", packet.c_str());
  DLOG_INFO(MOD_EB, "result:%d", res);
  return true;
}

void AsyncHttpSocket::LiveSignalClose(int error_code, bool is_signal) {
  AsyncHttpSocket::Ptr async_packet_socket = shared_from_this();
  SignalClose(error_code, is_signal);
}

void AsyncHttpSocket::SignalClose(int error_code, bool is_signal) {
  if (async_socket_) {
    async_socket_->Close();
    async_socket_.reset();
  }
  if (!SignalHttpPacketError.is_empty()) {
    if (is_signal) {
      SignalHttpPacketError(shared_from_this(), error_code);
    }
    SignalHttpPacketError.disconnect_all();
  }
  if (!SignalHttpPacketEvent.is_empty()) {
    SignalHttpPacketEvent.disconnect_all();
  }
  if (!SignalHttpPacketWrite.is_empty()) {
    SignalHttpPacketWrite.disconnect_all();
  }
  recv_size_ = 0;
}

void AsyncHttpSocket::OnAsyncSocketWriteEvent(AsyncSocket::Ptr socket) {
  AsyncHttpSocket::Ptr live_this = shared_from_this();
  SignalHttpPacketWrite(shared_from_this());
}

void AsyncHttpSocket::OnAsyncSocketReadEvent(AsyncSocket::Ptr socket,
    MemBuffer::Ptr data_buffer) {
  DLOG_INFO(MOD_EB, "Read data size: %d", data_buffer->size());
  AsyncHttpSocket::Ptr live_this = shared_from_this();
  if (!async_socket_ || async_socket_->IsClose()) {
    DLOG_ERROR(MOD_EB, "Socket is closed");
    return;
  }
  if (!AnalisysPacket(data_buffer)) {
    LiveSignalClose(1, true);
  } else {
    StartReadNextPacket();
  }
}

void AsyncHttpSocket::OnAsyncSocketErrorEvent(AsyncSocket::Ptr socket,
    int error_code) {
  LiveSignalClose(error_code, true);
}
////////////////////////////////////////////////////////////////////////////////
int AsyncHttpSocket::OnHttpMessageBegin(http_parser *parser) {
  DLOG_INFO(MOD_EB, "BEGIN");
  return 0;
}
int AsyncHttpSocket::OnHttpUrl(http_parser *parser,
                               const char *at,
                               std::size_t length) {
  DLOG_INFO(MOD_EB, "%s", at);
  http_req_message_.req_url.append(at, length);
  http_parser_url http_purl;
  http_parser_url_init(&http_purl);
  if (http_parser_parse_url(at, length, 0, &http_purl)) {
    return 1;
  }
  UrlFields &uf = http_req_message_.url_fields;

  if (http_purl.field_set & (1 << UF_SCHEMA)) {
    uf.schema.append(at + http_purl.field_data[UF_SCHEMA].off,
                     http_purl.field_data[UF_SCHEMA].len);
  }
  if (http_purl.field_set & (1 << UF_HOST)) {
    uf.host.append(at + http_purl.field_data[UF_HOST].off,
                   http_purl.field_data[UF_HOST].len);
  }
  if (http_purl.field_set & (1 << UF_PORT)) {
    uf.port.append(at + http_purl.field_data[UF_PORT].off,
                   http_purl.field_data[UF_PORT].len);
  }
  if (http_purl.field_set & (1 << UF_PATH)) {
    uf.path.append(at + http_purl.field_data[UF_PATH].off,
                   http_purl.field_data[UF_PATH].len);
  }
  if (http_purl.field_set & (1 << UF_QUERY)) {
    uf.query.append(at + http_purl.field_data[UF_QUERY].off,
                    http_purl.field_data[UF_QUERY].len);
    ParserRequestURL(at + http_purl.field_data[UF_QUERY].off,
                     http_purl.field_data[UF_QUERY].len,
                     http_req_message_.key_values);
  }
  if (http_purl.field_set & (1 << UF_FRAGMENT)) {
    uf.fragment.append(at + http_purl.field_data[UF_FRAGMENT].off,
                       http_purl.field_data[UF_FRAGMENT].len);
  }
  if (http_purl.field_set & (1 << UF_USERINFO)) {
    uf.userinfo.append(at + http_purl.field_data[UF_USERINFO].off,
                       http_purl.field_data[UF_USERINFO].len);
  }

  return 0;
}

void AsyncHttpSocket::ParserRequestURL(const char *req,
                                       uint32 size,
                                       KeyValues &key_values) {
  uint32 i = 0;
  while (i < size) {
    std::string key;
    std::string value;
    // Find key
    while (req[i] != '=' && i < size) {
      key.push_back(req[i]);
      i++;
    }
    i++;
    while (req[i] != '&' && i < size) {
      value.push_back(req[i]);
      i++;
    }
    i++;
    key_values.insert(std::pair<std::string, std::string>(key, value));
  }
}

int AsyncHttpSocket::OnHttpStatus(http_parser *parser,
                                  const char *at,
                                  std::size_t length) {
  DLOG_INFO(MOD_EB, "%s", at);
  return 0;
}
int AsyncHttpSocket::OnHttpHeaderField(http_parser *parser,
                                       const char *at,
                                       std::size_t length) {
  HttpHead hh;
  hh.name.append(at, length);
  http_req_message_.http_headers.push_back(hh);
  DLOG_INFO(MOD_EB, "%s", at);
  return 0;
}
int AsyncHttpSocket::OnHttpHeaderValue(http_parser *parser,
                                       const char *at,
                                       std::size_t length) {
  DLOG_INFO(MOD_EB, "%s", at);
  HttpHead &hh = http_req_message_.http_headers.back();
  hh.value.append(at, length);
  return 0;
}
int AsyncHttpSocket::OnHttpHeadersComplete(http_parser *parser) {
  http_req_message_.method = http_method_str((http_method)(parser->method));
  DLOG_INFO(MOD_EB, "%s", http_method_str((http_method)(parser->method)));
  return 0;
}
int AsyncHttpSocket::OnHttpBody(http_parser *parser,
                                const char *at,
                                std::size_t length) {
  DLOG_INFO(MOD_EB, "%s", at);
  http_req_message_.body.append(at, length);
  return 0;
}
int AsyncHttpSocket::OnHttpMessageComplete(http_parser *parser) {
  DLOG_INFO(MOD_EB, "OnHttpMessageComplete");
  SignalHttpPacketEvent(shared_from_this(), http_req_message_);
  return 0;
}

int AsyncHttpSocket::OnHttpChunkHeader(http_parser *parser) {
  DLOG_INFO(MOD_EB, "OnHttpChunkHeader");
  return 0;
}

int AsyncHttpSocket::OnHttpChunkComplete(http_parser *parser) {
  DLOG_INFO(MOD_EB, "OnHttpChunkComplete");
  return 0;
}
}  // namespace chml
