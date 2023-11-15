#ifndef EVENTSERVICE_MEM_MEMBUFFER_H_
#define EVENTSERVICE_MEM_MEMBUFFER_H_

#include "eventservice/base/basictypes.h"
#include "eventservice/base/atomic.h"

#include <list>
#include <vector>
#include "boost/boost_settings.hpp"

namespace chml {
#define ALIGN_WITH(x,a)    (((x)+(a)-1)&~(a-1))
// Block大小，必须为3的倍数，以适应Base64编码直接对block操作
#define DEFAULT_BLOCK_SIZE   4608 //768
extern volatile uint32 g_block_count;      //当前分配的MemBuffer Block数量
extern volatile uint32 g_block_count_max; //历史分配的MemBuffer Block最大数量
extern volatile uint32 g_membuffer_count; //当前分配的MemBuffer数量

class SharedBuffer : public boost::noncopyable {
 public:
  typedef boost::shared_ptr<SharedBuffer> Ptr;
  SharedBuffer() {
    buffer_ = new std::string();
    size_ = 0;
  }
  ~SharedBuffer() {
    delete buffer_;
  }
  inline void resize(size_t size) const { buffer_->resize(size); };
  inline void reserve(size_t size) {buffer_->reserve(size);};
  inline size_t capacity() const { return buffer_->capacity(); };
  inline std::string &buffer() const { return *buffer_; };
  inline void *data() const { return static_cast<void*>(&(*buffer_)[0]); };
  inline const char *c_str() {
    return buffer_->c_str();
  }
  size_t size(size_t w_size = 0) {
    assert(w_size >= 0);
    if (w_size > buffer_->capacity())
      buffer_->resize(w_size);
    if (w_size != 0)
      size_ = w_size;
    if (size_ == 0)
      size_ = buffer_->size();
    return size_;
  };

 private:
  std::string *buffer_;
  size_t size_;
};

struct Block : public boost::noncopyable {
  typedef boost::shared_ptr<Block> Ptr;
  Block() {
    buffer_size  = 0;
    buffer[0]    = 0;
    encode_flag_ = false;
    big_mem      = nullptr;
    ML_FAA(&g_block_count, 1);
    if (g_block_count > g_block_count_max) {
      g_block_count_max = g_block_count;
    }
  }

  ~Block() {
    ML_FAS(&g_block_count, 1);
  }
  size_t  RemainSize() const {
    return DEFAULT_BLOCK_SIZE - buffer_size;
  }

  static Block::Ptr TakeBlock();
  static void DumpBlocksInfo();

  size_t  WriteBytes(const char* val, size_t len);
  size_t  ReadBytes(char* val, size_t len);
  size_t  ReadString(std::string* val, size_t len);
  size_t  CopyBytes(size_t pos, char* val, size_t len);
  size_t  CopyString(size_t pos, std::string* val, size_t len);

  uint8   buffer[DEFAULT_BLOCK_SIZE];
  size_t  buffer_size;
  void*   big_mem;
  bool    encode_flag_;
};

typedef std::list<Block *> Blocks;
typedef std::list<Block::Ptr> BlocksPtr;

// 这个类一般适用于在大数据量传输通过种使用，目前只在两个地方使用
// 1. Filecahe存放图片的大数据应用
// 2. 网络数据传输
// 使用注意，多次写入少量数据的速度是非常慢的，尽量一次写入大量数据
class MemBuffer : public boost::noncopyable {
 public:
  typedef boost::shared_ptr<MemBuffer> Ptr;
  virtual ~MemBuffer();
  static MemBuffer::Ptr CreateMemBuffer();
 private:
  MemBuffer();
 public:
  size_t Length() const ;
  size_t size() const;
  size_t BlocksSize() const {
    return blocks_.size();
  }

  void DumpData();

  BlocksPtr &blocks() {
    return blocks_;
  };

  void AppendBuffer(MemBuffer::Ptr buffer);
  void AppendBlock(Block::Ptr block);
  void ReduceSize(size_t size) {
    size_ = size_ - size;
  }
  void Clear();

  // 开启\关闭数据编码标识。对MemBuffer中的所有Blocks设置该标识，在通过
  // NetWorkInterface AsyncWrite()发包时，如果Socket同时也设置了编码方式
  // （通过SetEncodeType()设置），则先对Block数据进行编码后再发送。
  bool EnableEncode(bool enable);

  // Copy some bytes from the buffer, not change the contents
  // of the buffer. Return false if there isn't enough data left
  // for the specified byte.
  bool CopyUInt8(size_t pos, uint8* val);
  bool CopyUInt16(size_t pos, uint16* val);
  bool CopyUInt32(size_t pos, uint32* val);
  bool CopyUInt64(size_t pos, uint64* val);
  bool CopyBytes(size_t pos, char* val, size_t len);
  bool CopyBuffer(MemBuffer::Ptr buffer, size_t len);
  // Copy some bytes from the buffer, not change the contents
  // of the buffer. If the actual length of the data is less
  // than the length to be read, return the actual length of
  // the data was copyed.
  size_t CopyBytes(char* val, size_t pos, size_t len);

  // Appends next |len| bytes from the buffer to |val|. Returns false
  // if there is less than |len| bytes left.
  bool CopyString(size_t pos, std::string* val, size_t len);

  // Read some bytes from the buffer, and remove the content being read.
  // Return false if there isn't enough data left for the specified byte.
  bool ReadUInt8(uint8* val);
  bool ReadUInt16(uint16* val);
  bool ReadUInt32(uint32* val);
  bool ReadUInt64(uint64* val);
  bool ReadBytes(char* val, size_t len);
  bool ReadBuffer(MemBuffer::Ptr buffer, size_t len);

  // Appends next |len| bytes from the buffer to |val|. Returns false
  // if there is less than |len| bytes left.
  bool ReadString(std::string* val, size_t len);

  std::string ToString();
  size_t size_by_encode();

  // Write value to the buffer. Resizes the buffer when it is
  // neccessary.
  void WriteUInt8(uint8 val);
  void WriteUInt16(uint16 val);
  void WriteUInt32(uint32 val);
  void WriteUInt64(uint64 val);
  void WriteString(const std::string& val);
  void WriteBytes(const char* val, size_t len);
  void WriteNewBytes(const char* val, size_t len);
  bool ReadFile(const char *file, off_t off, size_t len);
  //将buffer中的数据全读取到val中,如果val空间不足,返回-1,成功返回读取的实际长度，读取以后Buffer中就不会存在数据
  size_t ReadAllBytes(char* val, size_t len);
  static uint32 GetCurBlockCnt();
 private:
  BlocksPtr::iterator GetPostion(size_t pos, size_t *block_pos);

 private:
  size_t    size_;
  BlocksPtr blocks_;
  void WriteBigBytes(const char *val, size_t len);
};

}

#endif  // EVENTSERVICE_MEM_MEMBUFFER_H_

