#include "asyncpacketsocket.h"
#include "eventservice/log/log_client.h"

namespace chml {

AsyncPacketSocket::AsyncPacketSocket(EventService::Ptr event_service, AsyncSocket::Ptr socket)
  : async_socket_(socket) {
  ASSERT_RETURN_VOID(!socket);
  async_socket_->SignalSocketErrorEvent.connect(this, &AsyncPacketSocket::OnAsyncSocketErrorEvent);
  async_socket_->SignalSocketReadEvent.connect(this, &AsyncPacketSocket::OnAsyncSocketReadEvent);
  async_socket_->SignalSocketWriteEvent.connect(this, &AsyncPacketSocket::OnAsyncSocketWriteEvent);

  //////////////////////////////////////////////////////////////////////////////
  recv_buff_ = MemBuffer::CreateMemBuffer();
}

AsyncPacketSocket::~AsyncPacketSocket() {
  DLOG_WARNING(MOD_EB, "Destory async socket");
  SignalClose(0, false);
}

void AsyncPacketSocket::Close() {
  SignalClose(0, false);
}

const SocketAddress AsyncPacketSocket::local_addr() {
  if (async_socket_) return async_socket_->GetLocalAddress();
  return SocketAddress();
}

const SocketAddress AsyncPacketSocket::remote_addr() {
  if (async_socket_) return async_socket_->GetRemoteAddress();
  return SocketAddress();
}

std::string AsyncPacketSocket::ip_addr() {
  if (async_socket_) return async_socket_->GetRemoteAddress().HostAsURIString();
  return "";
}

bool  AsyncPacketSocket::IsOpen() const {
  if (async_socket_) return async_socket_->IsOpen();
  return false;
}

bool  AsyncPacketSocket::IsClose() {
  if (async_socket_) return async_socket_->IsClose();
  return false;
}

//Socket::Ptr AsyncPacketSocket::SocketNumber() {
//  BOOST_ASSERT(async_socket_);
//  return async_socket_->SocketNumber();
//}
////////////////////////////////////////////////////////////////////////////////
bool AsyncPacketSocket::AsyncWritePacket(const char *data, uint32 size, uint16 flag) {
  if (!async_socket_ || async_socket_->IsClose()) {
    DLOG_ERROR(MOD_EB, "Socket is closed");
    return false;
  }

  PacketHeader packet_header(flag, size);
  MemBuffer::Ptr data_buffer = MemBuffer::CreateMemBuffer();
  ASSERT_RETURN_FAILURE(!data_buffer, false);
  data_buffer->WriteBytes((const char *)&packet_header, PACKET_HEADER_SIZE);
  if ((data != NULL) && (size > 0)) {
    data_buffer->WriteBytes(data, size);
  }
  async_socket_->AsyncWrite(data_buffer);
  return true;
}

bool AsyncPacketSocket::AsyncWritePacket(const MemBuffer::Ptr &buffer, uint16 flag) {
  PacketHeader packet_header(flag, buffer->size());
  MemBuffer::Ptr send_buffer = MemBuffer::CreateMemBuffer();
  if (nullptr == send_buffer) {
    DLOG_ERROR(MOD_EB, "CreateMemBuffer failed.");
    return false;
  }

  chml::Block::Ptr block = chml::Block::TakeBlock();
  if (nullptr == block) {
    DLOG_ERROR(MOD_EB, "TakeBlock failed.");
    return false;
  }

  block->WriteBytes(reinterpret_cast<char*>(&packet_header), sizeof(PacketHeader));
  send_buffer->AppendBlock(block);
  send_buffer->AppendBuffer(buffer);

  if (async_socket_) {
    return async_socket_->AsyncWrite(send_buffer);
  }
  return false;
}

bool AsyncPacketSocket::AsyncRead() {
  if (!async_socket_ || async_socket_->IsClose()) {
    DLOG_ERROR(MOD_EB, "Socket is closed");
    return false;
  }
  return async_socket_->AsyncRead();
}

bool AsyncPacketSocket::AnalysisPacket(MemBuffer::Ptr buffer) {
  recv_buff_->AppendBuffer(buffer);

  while (true) {
    const uint32 recv_size = recv_buff_->size();
    if (recv_size < PACKET_HEADER_SIZE) {
      // Packet未接收完，头部信息不完整，将本次接收的数据存入接收缓存
      return true;
    }

    // 读取Packet头
    PacketHeader packet_header;
    bool bret = recv_buff_->CopyBytes(0,
                                      reinterpret_cast<char*>(&packet_header),
                                      PACKET_HEADER_SIZE);
    if (false == bret || false == packet_header.is_valid()) {
      DLOG_ERROR(MOD_EB, "Packet format error");
      return false;
    }

    const uint32 pack_size = packet_header.DataSize() + PACKET_HEADER_SIZE;
    if (pack_size > recv_size) {
      // Packet未接收完，将本次接收的数据存入接收缓存
      return true;
    } else {
      // Packet接收完整，处理粘包
      MemBuffer::Ptr usr_buff = MemBuffer::CreateMemBuffer();
      ASSERT_RETURN_FAILURE((nullptr == usr_buff), false);

      bret = recv_buff_->ReadBuffer(usr_buff, pack_size);
      if (bret) {
        bret = usr_buff->ReadBytes(NULL, PACKET_HEADER_SIZE);  // 去包头
        SignalPacketEvent(shared_from_this(), usr_buff, packet_header.Flag());
      }
    }
  }

  DLOG_ERROR(MOD_EB, "some error.");
  return false;
}

void AsyncPacketSocket::SignalClose(int error_code, bool is_signal) {
  if (async_socket_) {
    async_socket_->Close();
    async_socket_.reset();
  }
  if (!SignalPacketError.is_empty()) {
    if (is_signal) {
      SignalPacketError(shared_from_this(), error_code);
    }
    SignalPacketError.disconnect_all();
  }
  if (!SignalPacketEvent.is_empty()) {
    SignalPacketEvent.disconnect_all();
  }
  if (!SignalPacketWrite.is_empty()) {
    SignalPacketWrite.disconnect_all();
  }
}

void AsyncPacketSocket::LiveSignalClose(int error_code, bool is_signal) {
  AsyncPacketSocket::Ptr async_packet_socket = shared_from_this();
  SignalClose(error_code, is_signal);
}

void AsyncPacketSocket::OnAsyncSocketWriteEvent(AsyncSocket::Ptr socket) {
  SignalPacketWrite(shared_from_this());
}

void AsyncPacketSocket::OnAsyncSocketReadEvent(AsyncSocket::Ptr socket, MemBuffer::Ptr data_buffer) {
  if (!async_socket_ || async_socket_->IsClose()) {
    DLOG_ERROR(MOD_EB, "Socket is closed");
    return;
  }
  if (!AnalysisPacket(data_buffer)) {
    LiveSignalClose(1, true);
  } else {
    AsyncRead();
  }
}

void AsyncPacketSocket::OnAsyncSocketErrorEvent(AsyncSocket::Ptr socket,
    int error_code) {
  LiveSignalClose(error_code, true);
}

}  // namespace chml

