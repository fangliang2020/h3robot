#if 1  // shm_hash_map
#include "shmc/shm_hash_map.h"
#include <time.h>

const char* kShmKey = "0x100010";
const size_t kKeyNum = 300;
const size_t kNodeSize = 32;
const size_t kNodeNum = 2000;

int main(int argc, char* argv[]) {
  shmc::ShmHashMap<int32_t, shmc::SVIPC> kv_map_;

#if 1
  bool bret = kv_map_.InitForWrite(kShmKey, kKeyNum, kNodeSize, kNodeNum);
  printf("InitForWrite %d.\n", bret);

  srand(time(NULL));
  int arr[200] = { 0 };
  for (int i = 0; i < 200; i++) {
    arr[i] = i;
  }
  for (int i = 0; i < 200; i++) {
    int pos = rand() % 200;
    int temp = arr[i];
    arr[i] = arr[pos];
    arr[pos] = temp;
  }
  for (int i = 0; i < 200; i++) {
    std::string data = std::to_string(arr[i]);
    kv_map_.Replace(arr[i], data);
    printf("write ==> %03d -> %s.\n", arr[i], data.c_str());
  }

  while (kv_map_.size() > 0) {
    printf("*************** %d ******************\n", kv_map_.size());
    kv_map_.TravelEraseIf([](const int32_t& key, const std::string& data) -> bool {
      printf("erase ==> %03d -> %s.\n", key, data.c_str());
      return true;
    }, 10);
  }

  /*shmc::ShmHashMap<int32_t, shmc::SVIPC>::TravelPos pos;
  kv_map_.TravelEraseIf([](const int32_t& key, const std::string& data) -> bool {
    printf("read ==> %03d -> %s.\n", key, data.c_str());
    return false;
  }, 100);*/

#else
  if (argc >= 2 && 'w' == argv[1][0]) {
    int i = 0;
    bool bret = kv_map_.InitForWrite(kShmKey, kKeyNum, kNodeSize, kNodeNum);
    while (bret) {
      std::string data = (std::string("info") + std::to_string(time(NULL))).c_str();

      bret = kv_map_.Insert(i, data);

      printf("insert key %d, data %s, result %d press.\n", i, data.c_str(), bret);
      getchar();
      getchar();
      i++;
    }
  } else {
    int i = 0;
    bool bret = kv_map_.InitForRead(kShmKey);
    while (true) {
      std::string data;
      bret = kv_map_.Find(i, &data);

      printf("find key %d, data %s, result %d press.\n", i, data.c_str(), bret);
      getchar();
      getchar();
      i++;
    }
  }
#endif

  return EXIT_SUCCESS;
}
#endif

#if 0
#include <string>

#include "shmc/shm_hash_map.h"
#include "shmc/shm_hash_table.h"

const char* kShmKey = "0x100011";
const size_t kCapacity = 4 * 1024 * 1024;

struct Node {
  uint64_t key;
  uint64_t value;
  std::pair<bool, uint64_t> Key() const volatile {
    return std::make_pair((key != 0), key);
  }
  std::pair<bool, uint64_t> Key(uint64_t arg) const volatile {
    return std::make_pair((key > arg), key);
  }
};

int main(int argc, char* argv[]) {
  shmc::ShmHashTable<uint64_t, Node, shmc::SVIPC> kv_map_;

  if (argc >= 2 && 'w' == argv[1][0]) {
    int64_t i = 1;
    bool bret = kv_map_.InitForWrite(kShmKey, kCapacity);
    printf("%s[%d] ret %d.\n", __FUNCTION__, __LINE__, bret);
    while (bret) {
      bool is_found = false;
      volatile Node* node = kv_map_.FindOrAlloc(i, &is_found);
      if (node) {
        node->value = i;
      }
      printf("insert key %d, result %d press.\n", i, bret);
      getchar();
      getchar();
      i++;
    }
  } else {
    int i = 1;
    bool bret = kv_map_.InitForRead(kShmKey);
    printf("%s[%d] ret %d.\n", __FUNCTION__, __LINE__, bret);
    while (true) {
      bool is_found = false;
      volatile Node* node = kv_map_.FindOrAlloc(i, &is_found);
      if (node) {
        printf("find key %d, data %llu, result %d press.\n", i, node->value, bret);
      }

      getchar();
      getchar();
      i++;
    }
  }

  return EXIT_SUCCESS;
}
#endif

#if 0

#include "filecache_v2/owmr_queue.h"
#include "eventservice/event/thread.h"
#include "eventservice/base/timeutils.h"
#include "eventservice/util/string_util.h"

int main(int argc, char* argv[]) {
  cache::CowmrQueue cq;

  if (argc >= 2 && 'w' == argv[1][0])
  {
    cq.InitForWrite("0x100000", 1024 * 100);

    int32_t i = 0;
    do {
      uint64_t nmsec = chml::TimeMSecond();
      std::string rd = chml::XStrUtil::random_string(1024);
      int32_t nret = cq.Write(nmsec, rd.c_str(), rd.size());

      printf("write index %d[%llu] result %d.\n", i++, nmsec, nret);
      chml::Thread::SleepMs(1);
    } while (true);
  } else {
    cq.InitForRead("0x100000");

    int32_t i = 0;
    uint64_t nmsec = 0;
    do {
      char sdata[1025] = { 0 };
      nmsec += 1;
      int32_t nlen = cq.Read(nmsec, NULL, 0);
      printf("read length %d.\n", nlen);

      int32_t nret = cq.Read(nmsec, sdata, 1024);
      printf("read index %d[%llu] result %d.\n", i++, nmsec, nret);
      chml::Thread::SleepMs(1);
    } while (true);
  }

  return EXIT_SUCCESS;
}

#endif


#if 0

#include "shmc/svipc_shm_alloc.h"

int main(int argc, char *argv[]) {
  shmc::SVIPC smem;
  void *pmem = smem.Attach_AutoCreate("0x10000", 1024);
  printf("pmem %p. %s\n", pmem, pmem);

  snprintf((char*)pmem, 1024, "hello worlds");

  printf("pmem %p. %s\n", pmem, pmem);

  getchar();
  return EXIT_SUCCESS;
}


#endif
