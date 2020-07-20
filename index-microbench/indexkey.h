
#ifndef _INDEX_KEY_H
#define _INDEX_KEY_H

#include <cstring>
#include <string>
#include <iostream>

template <std::size_t keySize>
class GenericKey {
public:
  char data[keySize+1];
public:
  inline void setFromString(std::string key) {
    memset(data, 0, keySize);
    if(key.size() > keySize) {
      memcpy(data, key.c_str(), keySize);
      data[keySize] = '\0';
    } else {
      strcpy(data, key.c_str());
      data[key.size()] = '\0';
    }
    return;
  }

  inline void setFromStringUUID(std::string key) {
    // memset(data, 0, keySize);
    // if(key.size() > keySize) {
    //   memcpy(data, key.c_str(), keySize);
    //   data[keySize] = '\0';
    // } else {
    //   strcpy(data, key.c_str());
    //   data[key.size()] = '\0';
    // }
    // return;


    // uint64_t v = 0;
    // for (int i = 0; i < 16; ++i)
    //     v = 10*v+(str[str.size()-1-i]-'0');
    // s.val[1] = v;
    // v = 0;
    // for (int i = 16; i < 32 && str.size() >= i+1; ++i)
    //     v = 10*v+(str[str.size()-1-i]-'0');
    // s.val[1] = v;
    // return is;
  }

  // Constructor - Fills it with 0x00
  // This is for the skiplist to initialize an empty node
  GenericKey(int) { memset(data, 0x00, keySize); }
  GenericKey() { memset(data, 0x00, keySize); }
  // Copy constructor
  GenericKey(const GenericKey &other) { memcpy(data, other.data, keySize); }
  inline GenericKey &operator=(const GenericKey &other) {
    memcpy(data, other.data, keySize);
    return *this;
  }

  inline bool operator<(const GenericKey<keySize> &other) const { return strcmp(data, other.data) < 0; }
  inline bool operator>(const GenericKey<keySize> &other) const { return strcmp(data, other.data) > 0; }
  inline bool operator==(const GenericKey<keySize> &other) const { return strcmp(data, other.data) == 0; }
  // Derived operators
  inline bool operator!=(const GenericKey<keySize> &other) const { return !(*this == other); }
  inline bool operator<=(const GenericKey<keySize> &other) const { return !(*this > other); }
  inline bool operator>=(const GenericKey<keySize> &other) const { return !(*this < other); }

  template <std::size_t _keySize>
  friend std::istream& operator>>(std::istream &is, GenericKey<_keySize> &s);

  template <std::size_t _keySize>
  friend std::ostream& operator<<(std::ostream &os, const GenericKey<_keySize> &s);
};

template <std::size_t keySize>
std::istream& operator>>(std::istream &is, GenericKey<keySize> &s) {
    std::string str;
    is >> str;
    s.setFromString(str);
    return is;
}

template <std::size_t keySize>
std::ostream& operator<<(std::ostream &os, const GenericKey<keySize> &s) {
    size_t n = 0;
    for (size_t n = 0; s.data[n] && n <= keySize; n++)
      os << s.data[n];
    std::cerr << "Read: ";
    for (size_t n = 0; s.data[n] && n <= keySize; n++)
      std::cerr << s.data[n];
    std::cerr << std::endl; 
    return os;
}


// std::ostream& operator<<(std::ostream &os, GenericKey<keySize> &s) {
//     return os << s.val[0] << s.val[1];
// }

template <std::size_t keySize>
class GenericComparator {
public:
  GenericComparator() {}

  inline bool operator()(const GenericKey<keySize> &lhs, const GenericKey<keySize> &rhs) const {
    int diff = strcmp(lhs.data, rhs.data);
    return diff < 0;
  }
};

template <std::size_t keySize>
class GenericEqualityChecker {
public:
  GenericEqualityChecker() {}

  inline bool operator()(const GenericKey<keySize> &lhs, const GenericKey<keySize> &rhs) const {
    int diff = strcmp(lhs.data, rhs.data);
    return diff == 0;
  }
};

template <std::size_t keySize>
class GenericHasher {
public:
  GenericHasher() {}

  inline size_t operator()(const GenericKey<keySize> &lhs) const {
    (void)lhs;
    return 0UL;
  }
};

#endif
