#include <vector>
#include "gtestx/gtestx.h" 
#include "shmc/shm_array.h"

namespace {

constexpr const char* kShmKey = "0x10006";
constexpr size_t kSize = 1000000;

using TestTypes = testing::Types<shmc::POSIX, shmc::SVIPC, shmc::SVIPC_HugeTLB,
                                 shmc::ANON, shmc::HEAP>;

struct Node {
  uint32_t id;
  uint32_t val;
} __attribute__((packed));

}  // namespace

template <class Alloc>
class ShmArrayTest : public testing::Test {
 protected:
  virtual ~ShmArrayTest() {}
  virtual void SetUp() {
    shmc::SetLogHandler(shmc::kDebug, [](shmc::LogLevel lv, const char* s) {
      fprintf(stderr, "[%d] %s", lv, s);
    });
    this->alloc_.Unlink(kShmKey);
    shmc::Utils::SetDefaultCreateFlags(shmc::kCreateIfNotExist);
    ASSERT_TRUE(this->array_.InitForWrite(kShmKey, kSize));
    if (shmc::impl::AllocTraits<Alloc>::is_named) {
      this->array_ro_ = &this->array2_;
      ASSERT_TRUE(this->array_ro_->InitForRead(kShmKey));
    } else {
      this->array_ro_ = &this->array_;
    }
  }
  virtual void TearDown() {
    this->alloc_.Unlink(kShmKey);
    shmc::Utils::SetDefaultCreateFlags(shmc::kCreateIfNotExist);
  }
  shmc::ShmArray<Node, Alloc> array_;
  shmc::ShmArray<Node, Alloc> array2_;
  shmc::ShmArray<Node, Alloc>* array_ro_;
  Alloc alloc_;
};
TYPED_TEST_CASE(ShmArrayTest, TestTypes);

TYPED_TEST(ShmArrayTest, InitAndRW) {
  for (size_t i = 0; i < kSize; i++) {
    this->array_[i].id = i;
  }
  const shmc::ShmArray<Node, TypeParam>& array_ro = *this->array_ro_;
  for (size_t i = 0; i < kSize; i++) {
    auto id = array_ro[i].id;
    ASSERT_EQ(i, id);
  }
}

TYPED_TEST(ShmArrayTest, CreateFlags) {
  if (shmc::impl::AllocTraits<TypeParam>::is_named) {
    shmc::ShmArray<Node, TypeParam> array_new;
    ASSERT_FALSE(array_new.InitForWrite(kShmKey, kSize*2));
    shmc::Utils::SetDefaultCreateFlags(shmc::kCreateIfExtending);
    ASSERT_TRUE(array_new.InitForWrite(kShmKey, kSize*2));
  }
}

