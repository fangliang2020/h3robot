#ifndef EVENTSERVICE_NET_ASYNC_PACKET_SOCKET_H_
#define EVENTSERVICE_NET_ASYNC_PACKET_SOCKET_H_


#include "eventservice/net/eventservice.h"
#include "eventservice/net/networktinterface.h"
#include "eventservice/base/byteorder.h"

namespace chml {

typedef struct _PacketHeader PacketHeader;
#define PACKET_HEADER_MARK     0x5A5A5A5AAA55AA55
#define PACKET_HEADER_SIZE     PacketHeader::HeadSize()
#define PACKET_RECV_BUFF_SIZE  (64 * 1024)
#define PACKET_BODY_SIZE       (PACKET_RECV_BUFF_SIZE - PACKET_HEADER_SIZE)

typedef struct _PacketHeader {
  _PacketHeader() {
    m = 'M';
    l = 'L';
    flag = 0;
    data_size = 0;
  }
  _PacketHeader(uint16 _flag, uint32 _size) {
    m = 'M';
    l = 'L';
    flag = HostToNetwork16(_flag);
    data_size = HostToNetwork32(_size);
  }

  bool is_valid() const {
    if ('M' == m && 'L' == l) {
      return true;
    }
    return false;
  }

  static uint32 HeadSize() {
    return sizeof(PacketHeader);
  }

  uint16 Flag() const {
    return NetworkToHost16(flag);
  }
  uint32 DataSize() const {
    return NetworkToHost32(data_size);
  }

  uint8   m;
  uint8   l;
  uint16  flag;
  uint32  data_size;
} PacketHeader;

class AsyncPacketSocket : public boost::noncopyable,
  public boost::enable_shared_from_this<AsyncPacketSocket>,
  public sigslot::has_slots<> {

 public:
  typedef boost::shared_ptr<AsyncPacketSocket> Ptr;
  sigslot::signal3<AsyncPacketSocket::Ptr, MemBuffer::Ptr, uint16>  SignalPacketEvent;
  sigslot::signal2<AsyncPacketSocket::Ptr, int>                     SignalPacketError;
  sigslot::signal1<AsyncPacketSocket::Ptr>                          SignalPacketWrite;

 public:
  AsyncPacketSocket(EventService::Ptr event_service, AsyncSocket::Ptr socket);
  virtual ~AsyncPacketSocket();

 public:
  bool AsyncWritePacket(const char *data, uint32 size, uint16 flag);
  bool AsyncWritePacket(const MemBuffer::Ptr &buffer, uint16 flag);
  bool AsyncRead();

  uint32 SendBlock() {
    return async_socket_->GetBlockCnt();
  }
  uint32 RecvBlock() {
    return recv_buff_->BlocksSize();
  }

  virtual void        Close();
  const SocketAddress local_addr();
  const SocketAddress remote_addr();
  std::string         ip_addr();
  bool                IsOpen() const;
  bool                IsClose();

 private:
  bool AnalysisPacket(MemBuffer::Ptr buffer);
  void SignalClose(int error_code, bool is_signal);
  void LiveSignalClose(int error_code, bool is_signal);

  void OnAsyncSocketWriteEvent(AsyncSocket::Ptr socket);
  void OnAsyncSocketReadEvent(AsyncSocket::Ptr socket, MemBuffer::Ptr data_buffer);
  void OnAsyncSocketErrorEvent(AsyncSocket::Ptr socket, int error_code);

 private:
  AsyncSocket::Ptr async_socket_;
  MemBuffer::Ptr   recv_buff_;
};

}  // namespace chml

#endif  // EVENTSERVICE_NET_ASYNC_PACKET_SOCKET_H_
