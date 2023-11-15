#include "astring.h"
#include <cstdio>
#include <climits>
#include <cstddef>
#include <cstring>
#include <malloc.h>
#include <iostream>

#ifndef MAX_SIZE_T
#define MAX_SIZE_T           (~(size_t)0)
#endif

namespace chml {

unsigned int string::mTotalSize = 0;
static char kEmptyString[1] = { '\0' };

// Implementation of the std::string class.
//
// mData points either to a heap allocated array of bytes or the constant
// kEmptyString when empty and reserve has not been called.
//
// The size of the buffer pointed by mData is mCapacity + 1.
// The extra byte is need to store the '\0'.
//
// mCapacity is either mLength or the number of bytes reserved using
// reserve(int)).
//
// mLength is the number of char in the string, excluding the terminating '\0'.
//
// TODO: replace the overflow checks with some better named macros.
//
// Allocate n + 1 number of bytes for the string. Update mCapacity.
// Ensure that mCapacity + 1 and mLength + 1 is accessible.
// In case of error the original state of the string is restored.
// @param n Number of bytes requested. String allocate n + 1 to hold
//            the terminating '\0'.
// @return true if the buffer could be allocated, false otherwise.
bool string::SafeMalloc(size_type n) {
  // Not empty and no overflow
  if (n > 0 && n + 1 > n) {
    value_type *oldData = mData;

    mData = static_cast<value_type *>(malloc(n + 1));
    if (NULL != mData) {
      mCapacity = n;
      mTotalSize += mCapacity;
      return true;
    }
    mData = oldData;  // roll back
  }
  return false;
}

// Resize the buffer pointed by mData if n >= mLength.
// mData points to an allocated buffer or the empty string.
// @param n The number of bytes for the internal buffer.
//            Must be > mLength and > 0.
void string::SafeRealloc(size_type n) {
  // truncation or nothing to do or n too big (overflow)
  if (n < mLength || n == mCapacity || n + 1 < n) {
    return;
  }

  if (kEmptyString == mData) {
    if (SafeMalloc(n)) {
      *mData = '\0';
    }
    return;
  }

  mTotalSize -= mCapacity;

  value_type *oldData = mData;
  mData = static_cast<char*>(realloc(mData, n + 1));
  if (NULL == mData) { // reallocate failed.
    mData = oldData;
  } else {
    mCapacity = n;
    mTotalSize += mCapacity;
  }
}

void string::SafeFree(value_type *buffer) {
  if (buffer != kEmptyString) {
    free(buffer);
  }
}

// If the memory is on the heap, release it. Do nothing we we point at the empty
// string. On return mData points to str.
void string::ResetTo(value_type *str) {
  SafeFree(mData);
  mData = str;
}

void string::ConstructEmptyString() {
  mData = kEmptyString;
  mLength = 0;
  mCapacity = 0;
  mReuseMem = false;
}

void string::Constructor(const value_type *str, size_type pos, size_type n) {
  // Enough data and no overflow
  if (SafeMalloc(n)) {
    memcpy(mData, str + pos, n);
    mLength = n;
    mData[mLength] = '\0';
    mReuseMem = false;
    return;  // Success
  }
  ConstructEmptyString();
}

void string::Constructor(size_type n, char c) {
  // Enough data and no overflow

  if (SafeMalloc(n)) {
    memset(mData, c, n);
    mLength = n;
    mData[mLength] = '\0';
    mReuseMem = false;
    return;  // Success
  }
  ConstructEmptyString();
}

void string::DumpStringSize() {
  printf(">> mlstd::string is used size %u.\n", mTotalSize);
}

string::string() {
  ConstructEmptyString();
}

string::string(const string& str, size_type pos, size_type n) {
  if (pos < str.mLength) {
    if (npos == n) {
      Constructor(str.mData, pos, str.mLength - pos);
      return;
    } else if (n <= (str.mLength - pos)) {
      Constructor(str.mData, pos, n);
      return;
    }
  }
  ConstructEmptyString();
}

string::string(const value_type *str) {
  if (NULL != str) {
    Constructor(str, 0, strlen(str));
  } else {
    ConstructEmptyString();
  }
}

string::string(const value_type *str, size_type n) {
  if (npos != n) {
    Constructor(str, 0, n);
  } else {
    ConstructEmptyString();  // standard requires we throw length_error here.
  }
}

// Char repeat constructor.
string::string(size_type n, char c) {
  Constructor(n, c);
}

string::string(const value_type *begin, const value_type *end) {
  if (begin < end) {
    Constructor(begin, 0, end - begin);
  } else {
    ConstructEmptyString();
  }
}

string::~string() {
  clear();  // non virtual, ok to call.
}

void string::resize(size_type n) {
  SafeRealloc(n);

  mLength = n;
  mData[mLength] = '\0';
}

void string::clear() {
  mTotalSize -= mCapacity;

  mCapacity = 0;
  mLength = 0;
  ResetTo(kEmptyString);
}


string& string::erase(size_type pos, size_type n) {
  if (pos >= mLength || 0 == n) {
    return *this;
  }
  // start of the characters left which need to be moved down.
  const size_t remainder = pos + n;

  // Truncate, even if there is an overflow.
  if (remainder >= mLength || remainder < pos) {
    *(mData + pos) = '\0';
    mLength = pos;
    return *this;
  }
  // remainder < mLength and allocation guarantees to be at least
  // mLength + 1
  size_t left = mLength - remainder + 1;
  value_type *d = mData + pos;
  value_type *s = mData + remainder;
  memmove(d, s, left);
  mLength -= n;
  return *this;
}

void string::Append(const value_type *str, size_type n) {
  const size_type total_len = mLength + n;

  // n > 0 and no overflow for the string length + terminating null.
  if (n > 0 && (total_len + 1) > mLength) {
    if (total_len > mCapacity) {
      reserve(total_len);
      if (total_len > mCapacity) {
        // something went wrong in the reserve call.
        return;
      }
    }
    memcpy(mData + mLength, str, n);
    mLength = total_len;
    mData[mLength] = '\0';
  }
}

string& string::append(const value_type *str) {
  if (NULL != str) {
    Append(str, strlen(str));
  }
  return *this;
}

string& string::append(const value_type *str, size_type n) {
  if (NULL != str) {
    Append(str, n);
  }
  return *this;
}

string& string::append(const value_type *str, size_type pos, size_type n) {
  if (NULL != str) {
    Append(str + pos, n);
  }
  return *this;
}

string& string::append(const string& str) {
  Append(str.mData, str.mLength);
  return *this;
}

void string::push_back(const char c) {
  // Check we don't overflow.
  if (mLength + 2 > mLength) {
    const size_type total_len = mLength + 1;

    if (total_len > mCapacity) {
      reserve(total_len);
      if (total_len > mCapacity) {
        // something went wrong in the reserve call.
        return;
      }
    }
    *(mData + mLength) = c;
    ++mLength;
    mData[mLength] = '\0';
  }
}


int string::compare(const string& other) const {
  if (this == &other) {
    return 0;
  } else if (mLength == other.mLength) {
    return memcmp(mData, other.mData, mLength);
  } else {
    return mLength < other.mLength ? -1 : 1;
  }
}

int string::compare(const value_type *other) const {
  if (NULL == other) {
    return 1;
  }
  return strcmp(mData, other);
}

bool operator==(const string& left, const string& right) {
  if (&left == &right) {
    return true;
  } else {
    // We can use strcmp here because even when the string is build from an
    // array of char we insert the terminating '\0'.
    return strcmp(left.mData, right.mData) == 0;
  }
}

bool operator==(const string& left, const string::value_type *right) {
  if (NULL == right) {
    return false;
  }
  // We can use strcmp here because even when the string is build from an
  // array of char we insert the terminating '\0'.
  return std::strcmp(left.mData, right) == 0;
}

bool operator<(const string& left, const string& right) {
  return std::strcmp(left.mData, right.mData) < 0;
}

bool operator>(const string& left, const string& right) {
  return std::strcmp(left.mData, right.mData) > 0;
}

void string::reserve(size_type size) {
  if (0 == size) {
    if (0 == mCapacity) {
      return;
    } else if (0 == mLength) {
      // Shrink to fit an empty string.
      mCapacity = 0;
      ResetTo(kEmptyString);
    } else {
      // Shrink to fit a non empty string
      SafeRealloc(mLength);
    }
  } else if (size > mLength) {
    SafeRealloc(size);
  }
}

void string::reusemem(bool reuse) {
  mReuseMem = reuse;
}

void string::swap(string& other) {
  if (this == &other) return;
  value_type *const tmp_mData = mData;
  const size_type tmp_mCapacity = mCapacity;
  const size_type tmp_mLength = mLength;

  mData = other.mData;
  mCapacity = other.mCapacity;
  mLength = other.mLength;

  other.mData = tmp_mData;
  other.mCapacity = tmp_mCapacity;
  other.mLength = tmp_mLength;
}

const char& string::operator[](const size_type pos) const {
  return mData[pos];
}

char& string::operator[](const size_type pos) {
  return mData[pos];
}

string& string::assign(const string& str) {
  if (mReuseMem) {
    if (str.mLength <= mCapacity) {
      memcpy(mData, str.mData, str.mLength);
      mLength = str.mLength;
      mData[mLength] = '\0';
      return *this;
    }
  }
  clear();
  Constructor(str.mData, 0, str.mLength);
  return *this;
}

string& string::assign(const string& str, size_type pos, size_type n) {
  if (pos >= str.mLength) {
    // pos is out of bound
    return *this;
  }
  if (n <= str.mLength - pos) {
    if (mReuseMem) {
      if (n <= mCapacity) {
        memcpy(mData, str.mData + pos, n);
        mLength = n;
        mData[mLength] = '\0';
        return *this;
      }
    }
    clear();
    Constructor(str.mData, pos, n);
  }
  return *this;
}

string& string::assign(const value_type *str) {
  if (NULL == str) {
    return *this;
  }
  if (mReuseMem) {
    if (strlen(str) <= mCapacity) {
      memcpy(mData, str, strlen(str));
      mLength = strlen(str);
      mData[mLength] = '\0';
      return *this;
    }
  }
  clear();
  Constructor(str, 0, strlen(str));
  return *this;
}

string& string::assign(const value_type *array, size_type n) {
  if (NULL == array || 0 == n) {
    return *this;
  }
  if (mReuseMem) {
    if (n <= mCapacity) {
      memcpy(mData, array, n);
      mLength = n;
      mData[mLength] = '\0';
      return *this;
    }
  }
  clear();
  Constructor(array, 0, n);
  return *this;
}

string& string::operator=(char c) {
  clear();
  Constructor(1, c);
  return *this;
}

std::ostream& operator<<(std::ostream& output, const string& string) {
  output << string.mData;
  return output;
}

std::istream& operator>>(std::istream& input, string& str) {
  char s[1024];
  input >> s;
  str = s;
  return input;
}

string::size_type string::find(const value_type *str, size_type pos) const {
  if (NULL == str) {
    return string::npos;
  }

  // Empty string is found everywhere except beyond the end. It is
  // possible to find the empty string right after the last char,
  // hence we used mLength and not mLength - 1 in the comparison.
  if (*str == '\0') {
    return pos > mLength ? string::npos : pos;
  }

  if (mLength == 0 || pos > mLength - 1) {
    return string::npos;
  }

  value_type *idx = std::strstr(mData + pos, str);

  if (NULL == idx) {
    return string::npos;
  }
  return static_cast<size_type>(idx - mData);
}

string::size_type string::find_last_not_of(const string& s,
    size_type off) const {
  size_type offset = this->size();
  if (offset) {
    if (--offset > off)
      offset = off;
    do {
      if (s.size() > (this->size() - offset)) {
        continue;
      }
      int ret = memcmp(mData+offset, s.c_str(), s.size());
      if (ret != 0)
        return offset;
    } while (offset--);
  }
  return npos;
}

}  // namespace mlstd
