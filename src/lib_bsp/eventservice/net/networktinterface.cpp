#include "eventservice/net/networktinterface.h"

namespace chml {

void AsyncSocket::RemoveAllSignal() {
  if (!SignalSocketErrorEvent.is_empty()) {
    SignalSocketErrorEvent.disconnect_all();
  }
  if (!SignalSocketWriteEvent.is_empty()) {
    SignalSocketWriteEvent.disconnect_all();
  }
  if (!SignalSocketReadEvent.is_empty()) {
    SignalSocketReadEvent.disconnect_all();
  }
}

bool AsyncSocket::AsyncWrite(const char *data, std::size_t size) {
  // ASSERT_RETURN_FAILURE(size > DEFAULT_BLOCK_SIZE, false);
  MemBuffer::Ptr data_buffer = MemBuffer::CreateMemBuffer();
  data_buffer->WriteBytes(data, size);
  return AsyncWrite(data_buffer);
}

////////////////////////////////////////////////////////////////////////////////
void AsyncUdpSocket::RemoveAllSignal() {
  if (!SignalSocketErrorEvent.is_empty()) {
    SignalSocketErrorEvent.disconnect_all();
  }
  if (!SignalSocketWriteEvent.is_empty()) {
    SignalSocketWriteEvent.disconnect_all();
  }
  if (!SignalSocketReadEvent.is_empty()) {
    SignalSocketReadEvent.disconnect_all();
  }
}

bool AsyncUdpSocket::SendTo(const char *data,
                            std::size_t size,
                            const SocketAddress &addr) {
  if (!data) return false;
  MemBuffer::Ptr data_buffer = MemBuffer::CreateMemBuffer();
  data_buffer->WriteBytes(data, size);
  return SendTo(data_buffer, addr);
}

}  // namespace chml
