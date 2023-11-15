#include <string.h>
#include <stdio.h>
#include "eventservice/mem/membuffer.h"
// #include "eventservice/base/queue.h"
#include "eventservice/base/basictypes.h"
#include "eventservice/base/criticalsection.h"
#ifdef ENABLE_VTY
#include "eventservice/vty/vty_client.h"
#endif
#include "eventservice/log/log_client.h"
#include "eventservice/util/base64.h"
#include "eventservice/util/file_opera.h"
#include <map>
// Block缓存Queue大小
#ifdef ENABLE_MEMBUFF_BLOCK_CACHE
#define BLOCK_CACHE_SIZE  (1024)
#else
#define BLOCK_CACHE_SIZE  (0)
#endif

namespace chml {

#ifdef ENABLE_VTY
static void VtyRegistCmd();
#endif

volatile uint32 g_block_count      = 0; //当前分配的MemBuffer Block数量
volatile uint32 g_block_count_max  = 0; //历史分配的MemBuffer Block最大数量
volatile uint32 g_membuffer_count  = 0; //当前分配的MemBuffer数量

class BlockManager : public boost::noncopyable {
 public:
  static BlockManager *Instance();
  Block::Ptr TakeBlock() {
    chml::CritScope cr(&crit_);
    Block *block = NULL;
    if (blocks_.size() != 0) {
      block = blocks_.front();
      blocks_.pop_front();
    } else {
      block = new Block();
    }
    return Block::Ptr(block, BlockManager::RecyleBlock);
  }
  static void RecyleBlock(void *block);
 public:
  // 回收queue中缓存的blocks
  void InternalRecyleBlock(Block *block) {
    chml::CritScope cr(&crit_);
    block->buffer[0]    = 0;
    block->buffer_size  = 0;
    block->encode_flag_ = false;
    if (blocks_.size() < BLOCK_CACHE_SIZE) {
      blocks_.push_back(block);
    } else {
      delete block;
    }
  }
  BlockManager() {
#ifdef ENABLE_VTY
    VtyRegistCmd();
#endif
  }
  virtual ~BlockManager() {
  }

  uint32 CacheSize() {
    return blocks_.size();
  }
  void BigBlockRef(void *pmem, uint32_t cnt = 1);
  void BigBlockUnRef(void *pmem);
  static void FreeBigBlock(void *block);
 private:
  static BlockManager   *instance_;
  chml::CriticalSection  crit_;
  Blocks                 blocks_;
  std::map<void *, uint32_t>        big_mem_map_;
};

BlockManager *BlockManager::instance_ = NULL;
BlockManager *BlockManager::Instance() {
  if (instance_ == NULL) {
    instance_ = new BlockManager();
  }
  return instance_;
}

void BlockManager::RecyleBlock(void *block) {
  if (block != NULL) {
    BlockManager::Instance()->InternalRecyleBlock((Block *)block);
  } else {
    DLOG_ERROR(MOD_EB, "Block is null");
  }
}

void BlockManager::BigBlockRef(void *pmem, uint32_t cnt) {
  chml::CritScope cr(&crit_);
  big_mem_map_[pmem]+= cnt;
}

void BlockManager::BigBlockUnRef(void *pmem) {
  Block *blk = (Block *)pmem;
  chml::CritScope cr(&crit_);
  if(big_mem_map_.find(blk->big_mem) == big_mem_map_.end()){
    DLOG_ERROR(MOD_EB, "BigBlockUnRef %p not find", blk->big_mem);
    assert(false);
  }
  big_mem_map_[blk->big_mem]--;
  if(big_mem_map_[blk->big_mem] == 0) {
    assert(blk->big_mem != nullptr);
    big_mem_map_.erase(blk->big_mem);
    Block *ptr = (Block *)blk->big_mem;
    //printf("FlowCtrl delete mem %p\n", ptr);
    delete[] ptr;

  }
}

void BlockManager::FreeBigBlock(void *block) {
  Block *blk = (Block *)block;
  if (block != NULL) {
    BlockManager::Instance()->BigBlockUnRef((Block *)blk);
  } else {
    DLOG_ERROR(MOD_EB, "Block is null");
  }
}

#ifdef ENABLE_VTY
void VtyDumpBlockUsage(const void *vty_data, const void *user_data,
                       int argc, const char *argv[]) {
  Vty_Print(vty_data, ">> current MemBuffer count:%d, "
            "current Block count:%d, history Block max count:%d\n",
            g_membuffer_count, g_block_count, g_block_count_max);

  uint32 cache_size = BlockManager::Instance()->CacheSize();
  Vty_Print(vty_data, ">> Block cache total size:%d, current use count:%d\n",
            BLOCK_CACHE_SIZE, cache_size);
}

void VtyRegistCmd() {
  int res = Vty_RegistCmd("chml_membuff_block_info",
                          (VtyCmdHandler)chml::VtyDumpBlockUsage,
                          NULL,
                          "print membuff block usage info");
  if (0 != res) {
    DLOG_WARNING(MOD_EB, "register vty cmd failed:chml_membuff_block_info");
  }
}
#endif

////////////////////////////////////////////////////////////////////////////////
MemBuffer::Ptr MemBuffer::CreateMemBuffer() {
  return MemBuffer::Ptr(new MemBuffer());
}

MemBuffer::MemBuffer() : size_(0) {
  ML_FAA(&g_membuffer_count, 1);
}

MemBuffer::~MemBuffer() {
  ML_FAS(&g_membuffer_count, 1);
}

size_t MemBuffer::Length() const {
  return size_;
}

size_t MemBuffer::size() const {
  return size_;
}

// Read a next value from the buffer. Return false if there isn't
// enough data left for the specified type.
bool MemBuffer::ReadUInt8(uint8* val) {
  return ReadBytes((char *)val, sizeof(uint8));
}

bool MemBuffer::ReadUInt16(uint16* val) {
  return ReadBytes((char *)val, sizeof(uint16));
}

bool MemBuffer::ReadUInt32(uint32* val) {
  return ReadBytes((char *)val, sizeof(uint32));
}

bool MemBuffer::ReadUInt64(uint64* val) {
  return ReadBytes((char *)val, sizeof(uint64));
}

bool MemBuffer::ReadBytes(char* val, size_t len) {
  if (size_ < len) {
    return false;
  }
  size_t read_size = 0;
  for (BlocksPtr::iterator iter = blocks_.begin(); iter != blocks_.end();) {
    size_t rs = 0;
    if (val == NULL) {
      rs = (*iter)->ReadBytes(NULL, len - read_size);
    } else {
      rs = (*iter)->ReadBytes(val + read_size, len - read_size);
    }
    read_size += rs;
    if ((*iter)->buffer_size == 0) {
      iter = blocks_.erase(iter) ;
    } else {
      iter ++;
    }
    if (read_size == len) {
      break;
    }
  }
  size_ = size_ - len;
  return true;
}

bool MemBuffer::ReadBuffer(MemBuffer::Ptr buffer, size_t len) {
  if (size_ < len) {
    return false;
  }
  size_t read_size = 0;
  for (BlocksPtr::iterator iter = blocks_.begin(); iter != blocks_.end();) {
    size_t rs = 0;
    if (len - read_size >= (*iter)->buffer_size) {
      rs = (*iter)->buffer_size;
      buffer->AppendBlock(*iter);
      iter = blocks_.erase(iter);
    } else {
      rs = len - read_size;
      buffer->WriteBytes(reinterpret_cast<const char *>((*iter)->buffer), rs);
      (*iter)->ReadBytes(NULL, rs);
      ++iter;
    }
    read_size += rs;
    if (read_size == len) {
      break;
    }
  }
  size_ = size_ - len;
  return true;
}

// Appends next |len| bytes from the buffer to |val|. Returns false
// if there is less than |len| bytes left.
bool MemBuffer::ReadString(std::string* val, size_t len) {
  if (size_ < len) {
    return false;
  }
  size_t read_size = 0;
  for (BlocksPtr::iterator iter = blocks_.begin(); iter != blocks_.end();) {
    size_t rs = (*iter)->ReadString(val, len - read_size);
    read_size += rs;
    if ((*iter)->buffer_size == 0) {
      iter = blocks_.erase(iter) ;
    } else {
      iter ++;
    }
    if (read_size == len) {
      break;
    }
  }
  size_ = size_ - len;
  return true;
}


std::string MemBuffer::ToString() {
  std::string result = "";
  for (BlocksPtr::iterator iter = blocks_.begin(); iter != blocks_.end(); iter++) {
    if ((*iter)->encode_flag_) {
      std::string enc_str;
      chml::Base64::EncodeFromArray((*iter)->buffer, (*iter)->buffer_size, &enc_str);

      ///result += ;
      result.append(enc_str.c_str(), enc_str.size());
    } else {
      result.append((const char *)((*iter)->buffer), (*iter)->buffer_size);
    }
  }
  return result;
}

size_t MemBuffer::size_by_encode() {
  size_t buff_size = 0;
  for (BlocksPtr::iterator iter = blocks_.begin(); iter != blocks_.end(); iter++) {
    if ((*iter)->encode_flag_) {
      buff_size += chml::Base64::CalcDstSize((*iter)->buffer_size);
    } else {
      buff_size += (*iter)->buffer_size;
    }
  }
  return buff_size;
}

void MemBuffer::DumpData() {
  for (BlocksPtr::iterator iter = blocks_.begin();
       iter != blocks_.end(); iter++) {
    Log_HexDump(NULL, (const unsigned char*)((*iter)->buffer), (*iter)->buffer_size);
  }
}

// -----------------------------------------------------------------------------

void MemBuffer::AppendBuffer(MemBuffer::Ptr buffer) {
  BlocksPtr &blocks = buffer->blocks();
  blocks_.insert(blocks_.end(), blocks.begin(), blocks.end());
  size_ = size_ + buffer->size();
}

void MemBuffer::AppendBlock(Block::Ptr block) {
  blocks_.push_back(block);
  size_ = size_ + block->buffer_size;
}

void MemBuffer::Clear() {
  blocks_.clear();
  size_ = 0;
}

bool MemBuffer::EnableEncode(bool enable) {
  if (blocks_.size() == 0) {
    return false;
  }
  for (BlocksPtr::iterator iter = blocks_.begin(); iter != blocks_.end(); iter++) {
    Block::Ptr block = *iter;
    block->encode_flag_ = enable;
  }
  return true;
}

// Copy a next value from the buffer. Return false if there isn't
// enough data left for the specified type.
bool MemBuffer::CopyUInt8(size_t pos, uint8* val) {
  return CopyBytes(pos, (char *)val, sizeof(uint8));
}

bool MemBuffer::CopyUInt16(size_t pos, uint16* val) {
  return CopyBytes(pos, (char *)val, sizeof(uint16));
}

bool MemBuffer::CopyUInt32(size_t pos, uint32* val) {
  return CopyBytes(pos, (char *)val, sizeof(uint32));
}

bool MemBuffer::CopyUInt64(size_t pos, uint64* val) {
  return CopyBytes(pos, (char *)val, sizeof(uint64));
}

bool MemBuffer::CopyBytes(size_t pos, char* val, size_t len) {
  if (size_ - pos < len) {
    return false;
  }

  (void)CopyBytes(val, pos, len);
  return true;
}

bool MemBuffer::CopyBuffer(MemBuffer::Ptr buffer, size_t len) {
  if (size_ < len) {
    return false;
  }
  size_t read_size = 0;
  for (BlocksPtr::iterator iter = blocks_.begin(); iter != blocks_.end();) {
    size_t rs = ML_MIN(len - read_size, (*iter)->buffer_size);
    buffer->WriteBytes((const char *)((*iter)->buffer), rs);
    read_size += rs;
    if (read_size == len) {
      break;
    }
    iter++;
  }
  return true;
}

size_t MemBuffer::CopyBytes(char* val, size_t pos, size_t len) {
  if (pos >= size_) {
    return 0;
  }
  size_t act_len = ((pos + len) > size_) ? (size_ - pos) : (len);
  size_t block_pos = 0;
  BlocksPtr::iterator iter = GetPostion(pos, &block_pos);
  size_t total_cs = 0;
  while(iter != blocks_.end()) {
    size_t cs = (*iter)->CopyBytes(block_pos, val + total_cs, act_len - total_cs);
    total_cs += cs;
    if (total_cs == act_len) {
      break;
    }
    block_pos = 0;
    iter++;
  }
  return act_len;
}

// Appends next |len| bytes from the buffer to |val|. Returns false
// if there is less than |len| bytes left.
bool MemBuffer::CopyString(size_t pos, std::string* val, size_t len) {
  if (size_ - pos < len) {
    return false;
  }
  size_t block_pos = 0;
  BlocksPtr::iterator iter = GetPostion(pos, &block_pos);
  size_t total_cs = 0;
  while(iter != blocks_.end()) {
    size_t cs = (*iter)->CopyString(block_pos, val, len - total_cs);
    total_cs += cs;
    if (total_cs == len) {
      break;
    }
    block_pos = 0;
    iter++;
  }
  return true;
}

BlocksPtr::iterator MemBuffer::GetPostion(size_t pos, size_t *block_pos) {
  if (pos >= size_) {
    return blocks_.end();
  }
  size_t remain_pos = pos;
  for (BlocksPtr::iterator iter = blocks_.begin();
       iter != blocks_.end(); iter++) {
    int rs = (*iter)->buffer_size - remain_pos;
    if (rs > 0) {
      *block_pos = remain_pos;
      return iter;
    } else {
      remain_pos = remain_pos - (*iter)->buffer_size;
    }
  }
  return blocks_.end();
}

// -----------------------------------------------------------------------------
// Write value to the buffer. Resizes the buffer when it is
// neccessary.
void MemBuffer::WriteUInt8(uint8 val) {
  WriteBytes((const char *)&val, sizeof(uint8));
}

void MemBuffer::WriteUInt16(uint16 val) {
  WriteBytes((const char *)&val, sizeof(uint16));
}

void MemBuffer::WriteUInt32(uint32 val) {
  WriteBytes((const char *)&val, sizeof(uint32));
}

void MemBuffer::WriteUInt64(uint64 val) {
  WriteBytes((const char *)&val, sizeof(uint64));
}

void MemBuffer::WriteString(const std::string& val) {
  WriteBytes(val.c_str(), val.size());
}

void MemBuffer::WriteBigBytes(const char *val, size_t len) {
  uint32_t blk_cnt = len / DEFAULT_BLOCK_SIZE;
  if((len % DEFAULT_BLOCK_SIZE) > 0) {
    blk_cnt++;
  }

  Block *blk = new Block[blk_cnt];
  //printf("FlowCtrl new mem %p\n", blk);
  uint32_t wr_size = 0;
  for(uint32_t i = 0; i < blk_cnt; i++) {
    assert(wr_size < len);
    if(len - wr_size >= DEFAULT_BLOCK_SIZE) {
      blk[i].WriteBytes(val + wr_size, DEFAULT_BLOCK_SIZE);
      wr_size += DEFAULT_BLOCK_SIZE;
    } else {
      blk[i].WriteBytes(val + wr_size, len - wr_size);
      wr_size += (len - wr_size);
    }
    blk[i].big_mem = blk;
    blocks_.push_back(Block::Ptr(&blk[i], BlockManager::FreeBigBlock));
  }
  size_ += len;
  BlockManager::Instance()->BigBlockRef(blk, blk_cnt);
}

void MemBuffer::WriteBytes(const char* val, size_t len) {
  //DLOG_INFO(MOD_EVT, "FlowCtrl: WriteBytes %u", len);
  if(len > 32 * 1024) {
    return WriteBigBytes(val, len);
  }

  if (blocks_.size() == 0) {
    WriteNewBytes(val, len);
  } else {
    // 1. 先检查最后一个Buffer是否有可以写的空间
    Block::Ptr last_block = blocks_.back();
    size_t ws = 0;
    if (last_block->RemainSize() != 0) {
      ws = last_block->WriteBytes(val, len);
      size_ = size_ + ws;
    }
    // 2. 如果数据没有写完，就直接写新的Block
    if (len > ws) {
      WriteNewBytes(val + ws, len - ws);
    }
  }
  // size_ = size_ + len;
}

void MemBuffer::WriteNewBytes(const char* val, size_t len) {
  //DLOG_INFO(MOD_EVT, "FlowCtrl: WriteNewBytes %u", len);
  if(len > 32 * 1024) {
    return WriteBigBytes(val, len);
  }

  size_t pos = 0;
  while(true) {
    Block::Ptr block = BlockManager::Instance()->TakeBlock();
    if (NULL == block.get()) {
      LOG_POINT(167, 11, "Get Block Failed.");
      return;
    }
    
    blocks_.push_back(block);
    size_t ws = block->WriteBytes(val + pos, len - pos);
    size_ = size_ + ws;
    pos += ws;
    if (pos == len) {
      return ;
    }
  }
}


size_t MemBuffer::ReadAllBytes(char* val, size_t len) {
  if (len < size_) {
    return -1;
  }

  size_t read_size = 0;
  for (BlocksPtr::iterator iter = blocks_.begin(); iter != blocks_.end();) {
    size_t rs = 0;
    if (val == NULL) {
      rs = (*iter)->ReadBytes(NULL, len - read_size);
    }
    else {
      rs = (*iter)->ReadBytes(val + read_size, len - read_size);
    }
    read_size += rs;
    if ((*iter)->buffer_size == 0) {
      iter = blocks_.erase(iter);
    }
    else {
      iter++;
    }
    if (read_size == len) {
      break;
    }
  }
  size_ = size_ - read_size;
  return read_size;
}
uint32 MemBuffer::GetCurBlockCnt() {
  return ML_FAA(&g_block_count, 0);
}

bool MemBuffer::ReadFile(const char *file, off_t off, size_t len) {
  DLOG_ERROR(MOD_EVT, "FlowCtrl ReadFile %s off %d size %d", file, off, len);
  int fd = ::open(file, O_RDONLY);
  if (fd == -1) {
    DLOG_ERROR(MOD_EVT, "FlowCtrl open file wrong  %s wrong: %s", file, strerror(errno));
    return false;
  }

  off_t file_size = lseek(fd, 0, SEEK_END);
  if (file_size <= 0) {
    ::close(fd);
    DLOG_ERROR(MOD_EVT, "FlowCtrl lseek file %s wrong: %s", file, strerror(errno));
    return false;
  }

  if (off + len > file_size) {
    ::close(fd);
    DLOG_ERROR(MOD_EVT, "off + len > file_size %d %d %d", off, len, file_size);
    return false;
  }

  if (lseek(fd, off, SEEK_SET) < 0) {
    ::close(fd);
    DLOG_ERROR(MOD_EVT, "FlowCtrl lseek file %s wrong: %s", file, strerror(errno));
    return false;
  }

  uint32_t blk_cnt = len / DEFAULT_BLOCK_SIZE;
  if((len % DEFAULT_BLOCK_SIZE) > 0) {
    blk_cnt++;
  }

  Block *blk = new Block[blk_cnt];
  //printf("FlowCtrl new mem %p\n", blk);
  uint32_t wr_size = 0;
  for (uint32_t i = 0; i < blk_cnt; i++) {
    assert(wr_size < len);
    ssize_t ret = 0;
    if (len - wr_size >= DEFAULT_BLOCK_SIZE) {
      ret = read(fd, blk[i].buffer, DEFAULT_BLOCK_SIZE);
    } else {
      ret = read(fd, blk[i].buffer, len - wr_size);
    }
    if (ret <= 0) {
      DLOG_ERROR(MOD_EVT, "FlowCtrl read file %s failed %d.: %s", file, ret, strerror(errno));
      // warning , must revert  Blk
      delete[] blk;
      ::close(fd);
      return false;
    }
    blk[i].buffer_size = ret;
    blk[i].big_mem = blk;

    wr_size += DEFAULT_BLOCK_SIZE;
  }

  for (uint32_t i = 0; i < blk_cnt; i++) {
    blocks_.push_back(Block::Ptr(&blk[i], BlockManager::FreeBigBlock));
  }
  size_ += len;
  BlockManager::Instance()->BigBlockRef(blk, blk_cnt);
  DLOG_ERROR(MOD_EVT, "FlowCtrl read file %u ok", len);
  ::close(fd);
  return true;
}

////////////////////////////////////////////////////////////////////////////////
Block::Ptr Block::TakeBlock() {
  return BlockManager::Instance()->TakeBlock();
}

void Block::DumpBlocksInfo() {
  printf("FlowCtrl>> current MemBuffer count:%u, "
         "current Block count:%u, "
         "history Block max count:%u\n",
         g_membuffer_count, g_block_count, g_block_count_max);

  uint32 cache_size = BlockManager::Instance()->CacheSize();
  printf("FlowCtrl>> Block cache total size:%d, current use count:%d\n",
         BLOCK_CACHE_SIZE, cache_size);
}

size_t Block::WriteBytes(const char* val, size_t len) {
  if (DEFAULT_BLOCK_SIZE == buffer_size || len == 0) {
    return 0;
  }
  size_t remain_size = DEFAULT_BLOCK_SIZE - buffer_size;
  size_t write_size  = remain_size > len ? len : remain_size;
  memcpy(buffer + buffer_size, val, write_size);
  buffer_size += write_size;
  return write_size;
}

size_t Block::ReadBytes(char* val, size_t len) {
  if (0 == buffer_size || len == 0) {
    return 0;
  }
  if (buffer_size > DEFAULT_BLOCK_SIZE) {
    printf("ReadBytes buffer_size is large default %d.\n", buffer_size);
    return 0;
  }

  size_t remain_size = buffer_size;
  size_t read_size   = remain_size > len ? len : remain_size;
  if (val != NULL) {
    memcpy(val, buffer, read_size);
  }
  remain_size = buffer_size - read_size;
  memmove(buffer, buffer + read_size, remain_size);
  buffer_size = remain_size;
  return read_size;
}

size_t Block::ReadString(std::string* val, size_t len) {
  if (0 == buffer_size || len == 0) {
    return 0;
  }
  size_t remain_size = buffer_size;
  size_t read_size   = remain_size > len ? len : remain_size;
  val->append((const char *)buffer, read_size);
  remain_size = buffer_size - read_size;
  memmove(buffer, buffer + read_size, remain_size);
  buffer_size = remain_size;
  return read_size;
}

size_t Block::CopyBytes(size_t pos, char* val, size_t len) {
  if (pos > buffer_size || len == 0) {
    return 0;
  }
  size_t remain_size = buffer_size - pos;
  size_t copy_size   = remain_size > len ? len : remain_size;
  memcpy(val, buffer + pos, copy_size);
  return copy_size;
}

size_t Block::CopyString(size_t pos, std::string* val, size_t len) {
  if (pos > buffer_size || len == 0) {
    return 0;
  }
  size_t remain_size = buffer_size - pos;
  size_t copy_size   = remain_size > len ? len : remain_size;
  val->append((const char *)(buffer + pos), copy_size);
  return copy_size;
}

}


