#ifndef PRIME_KEY_PRIME_H_
#define PRIME_KEY_PRIME_H_

#include <string>

#include "eventservice/base/basictypes.h"

namespace prime {
// The digest array size must be big than 16 unsigned char
//
class PrimeGenerator {
 public:
  static void HmacMd5(const unsigned char *data, int data_size,
                      const unsigned char *key, int key_size,
                      unsigned char *digest);

  static const std::string GenerateMlPrimeKey(const unsigned char *data, int data_size);

  static void HmacSha1(const uint8* key, std::size_t key_size,
                       const uint8* data, std::size_t data_size, uint8* result);

  static void HmacSha1ToBase64(const std::string& key, const std::string& data, std::string& result);

};
}


#endif // PRIME_KEY_PRIME_H_