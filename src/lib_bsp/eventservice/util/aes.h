#ifndef AES_AES_H
#define AES_AES_H

#include <string>
#include <stdint.h>

// data length must be multiple of 16B, so data need to be padded before encryption/decryption
int aesEncrypt(const uint8_t *key, uint32_t keyLen, const uint8_t *pt, uint8_t *ct, uint32_t len);

int aesDecrypt(const uint8_t *key, uint32_t keyLen, const uint8_t *ct, uint8_t *pt, uint32_t len);

// ECB zero填充
int aesEncryptStr(std::string& src, std::string& dst, const uint8_t *key);

int aesDecryptStr(std::string& src, std::string& dst, const uint8_t *key);

#endif //AES_AES_H
