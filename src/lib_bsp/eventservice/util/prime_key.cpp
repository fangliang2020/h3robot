#include "prime_key.h"

#include <string.h>

#include <iostream>

#include "md5.h"
#include "sha1.h"

#include "eventservice/util/base64.h"

namespace prime {

void PrimeGenerator::HmacMd5(const unsigned char* data, int data_size,
                             const unsigned char* key, int key_size,
                             unsigned char* digest) {
  md5_state_t context;
  unsigned char k_ipad[65];    /* inner padding - key XORd with ipad */
  unsigned char k_opad[65];    /* outer padding - key XORd with opad */
  unsigned char tk[16];
  int i;
  /* if key is longer than 64 bytes reset it to key=MD5(key) */
  if (key_size > 64) {
    md5_state_t      tctx;

    md5_init(&tctx);
    md5_append(&tctx, key, key_size);
    md5_finish(&tctx, tk);

    key = tk;
    key_size = 16;
  }

  /*
  * the HMAC_MD5 transform looks like:
  *
  * MD5(K XOR opad, MD5(K XOR ipad, text))
  *
  * where K is an n byte key
  * ipad is the byte 0x36 repeated 64 times

  * opad is the byte 0x5c repeated 64 times
  * and text is the data being protected
  */

  /* start out by storing key in pads */
  memset(k_ipad, 0, sizeof(k_ipad));
  memset(k_opad, 0, sizeof(k_opad));
  memcpy(k_ipad, key, key_size);
  memcpy(k_opad, key, key_size);

  /* XOR key with ipad and opad values */
  for (i = 0; i < 64; i++) {
    k_ipad[i] ^= 0x36;
    k_opad[i] ^= 0x5c;
  }

  /* perform inner MD5 */
  md5_init(&context);                     /* init context for 1st pass */
  md5_append(&context, k_ipad, 64);       /* start with inner pad */
  md5_append(&context, data, data_size);  /* then text of datagram */
  md5_finish(&context, digest);           /* finish up 1st pass */

  /* perform outer MD5 */
  md5_init(&context);                     /* init context for 2nd pass */
  md5_append(&context, k_opad, 64);       /* start with outer pad */
  md5_append(&context, digest, 16);       /* then results of 1st hash */
  md5_finish(&context, digest);           /* finish up 2nd pass */
}

static const int MAX_DIGEST_SHORT_SIZE = 8;
static const char BASE62_SET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 "1234567890"
                                 "abcdefghijklmnopqrstuvwxyz";
static const int MAX_BASE62_SET_SIZE = 62;

const std::string ShortDigestToPrimeKey(
  unsigned short digest[MAX_DIGEST_SHORT_SIZE]) {
  std::string result;
  for (int i = 0; i < MAX_DIGEST_SHORT_SIZE; i++) {
    result.push_back(BASE62_SET[digest[i] % MAX_BASE62_SET_SIZE]);
  }
  return result;
}

void ToLowercase(const unsigned char* in_data, int in_size, unsigned char* out_data) {
  static unsigned const char LOWERCASE_OFFSET = 'Z' - 'z';
  for (int i = 0; i < in_size; i++) {
    if (in_data[i] >= '0' && in_data[i] <= '9') {
      out_data[i] = in_data[i];
    } else if (in_data[i] >= 'A' && in_data[i] <= 'Z') {
      out_data[i] = in_data[i] - LOWERCASE_OFFSET;
    } else {
      out_data[i] = in_data[i];
    }
#ifndef _DEBUG
    std::cout << out_data[i];
#endif
  }
#ifndef _DEBUG
  std::cout << std::endl;
#endif
}

const std::string PrimeGenerator::GenerateMlPrimeKey(
  const unsigned char* data, int data_size) {
  static const unsigned char MLENITH_INTERNAL_KEY[] = "mlenith";
  static const int MLENITH_INTERNAL_KEY_SIZE = 7;
  unsigned short digest[MAX_DIGEST_SHORT_SIZE];

  // New Memory
  unsigned char* lowcase_data = new unsigned char[data_size];

  // Conver the data to lowcase
  ToLowercase(data, data_size, lowcase_data);

  HmacMd5(lowcase_data, data_size, MLENITH_INTERNAL_KEY, MLENITH_INTERNAL_KEY_SIZE, (unsigned char*)digest);

  // delete the memory
  delete[] lowcase_data;
  lowcase_data = NULL;

  return ShortDigestToPrimeKey(digest);
}

void* memxor(void* dest, const void* src, size_t n) {
  char const* s = (const char*)src;
  char* d = reinterpret_cast<char*>(dest);

  for (; n > 0; n--)
    *d++ ^= *s++;

  return dest;
}

#define IPAD 0X36
#define OPAD 0X5C

void PrimeGenerator::HmacSha1(const uint8* key, std::size_t key_size,
                              const uint8* data, std::size_t data_size,
                              uint8* result) {
  SHA1_CTX inner;
  SHA1_CTX outer;
  unsigned char key_buf[64] = { 0 };
  char optkeybuf[20];
  char block[64];
  char innerhash[20];

  /* Reduce the key's size, so that it becomes <= 64 bytes large.  */

  if (key_size > 64) {
    SHA1_CTX keyhash;

    SHA1Init(&keyhash);
    SHA1Update(&keyhash, (const unsigned char*)key, key_size);
    SHA1Final(&keyhash, (unsigned char*)optkeybuf);

    memcpy(key_buf, optkeybuf, 20);
    key_size = 20;
  } else {
    memcpy(key_buf, key, key_size);
  }

  /* Compute INNERHASH from KEY and IN.  */

  SHA1Init(&inner);

  memset(block, IPAD, sizeof(block));
  memxor(block, key_buf, key_size);

  SHA1Update(&inner, (const unsigned char*)block, 64);
  SHA1Update(&inner, (const unsigned char*)data, data_size);

  SHA1Final(&inner, (unsigned char*)innerhash);

  /* Compute result from KEY and INNERHASH.  */

  SHA1Init(&outer);

  memset(block, OPAD, sizeof(block));
  memxor(block, key_buf, key_size);

  SHA1Update(&outer, (const unsigned char*)block, 64);
  SHA1Update(&outer, (const unsigned char*)innerhash, 20);

  SHA1Final(&outer, (unsigned char*)result);
}

void PrimeGenerator::HmacSha1ToBase64(const std::string& key, const std::string& data, std::string& result) {
  unsigned char digest_buffer[SHA1_DIGEST_SIZE];
  HmacSha1((const unsigned char*)key.c_str(), key.size(),
           (const unsigned char*)data.c_str(), data.size(), digest_buffer);

  chml::Base64::EncodeFromArray(digest_buffer, SHA1_DIGEST_SIZE, &result);
}

}