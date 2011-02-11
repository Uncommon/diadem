// Copyright 2010 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License.  You may obtain a copy
// of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations under
// the License.

#ifndef DIADEM_WRAPPERS_H_
#define DIADEM_WRAPPERS_H_

#ifndef DIADEM_HAVE_ARRAY
#include <vector>
#include <algorithm>
#endif

#ifndef DIADEM_HAVE_MAP
#include <map>
#endif

#ifndef DIADEM_HAVE_STACK
#include <stack>
#endif

#include "Diadem/Base.h"

#ifndef DIADEM_HAVE_ASSERT
#define DASSERT assert
#endif

namespace Diadem {

// Some projects may want to use their own container classes. In that case,
// you can create your own wrappers in a previously included header file
// and #define the appropriate symbols.

#ifndef DIADEM_HAVE_ARRAY
#define DIADEM_HAVE_ARRAY
template <class T>
class Array : public std::vector<T> {
  typedef std::vector<T> Inherited;
 public:
  void Remove(const T &t) {
    typename Inherited::iterator i =
        std::find(Inherited::begin(), Inherited::end(), t);
    if (i != Inherited::end())
      Inherited::erase(i);
  }
};
#endif

#ifndef DIADEM_HAVE_MAP
#define DIADEM_HAVE_MAP
template <class K, class V>
class Map : std::map<K, V> {
  typedef std::map<K, V> Inherited;
 public:
  typedef std::pair<K, V> Pair;

  void Insert(const K &key, const V &val)
    { Inherited::insert(std::make_pair(key, val)); }
  bool Exists(const K &key) const
    { return Inherited::find(key) != Inherited::end(); }

  using Inherited::operator[];
  V operator[](const K &key) const {
    typename Inherited::const_iterator i = Inherited::find(key);
    if (i == Inherited::end())
      return V();
    else
      return i->second;
  }

  Array<K> AllKeys() const {
    Array<K> result;
    for (typename Inherited::const_iterator i = Inherited::begin();
        i != Inherited::end(); ++i) {
      result.push_back(i->first);
    }
    return result;
  }
};
#endif

#ifndef DIADEM_HAVE_STACK
#define DIADEM_HAVE_STACK
template <class T>
class Stack : public std::stack<T> {};
#endif

// The String class is different from the above wrapper classes. It is intended
// for the simple use case of holding an immutable string, so it does not need
// to involve a more complex class like std::string.
class String : public Base {
 public:
  enum Adopt { kAdoptBuffer };

  String() : string_(NULL) { Clear(); }
  String(const char *s)   : string_(NULL) { *this = s; }
  String(const char *s, Adopt) : string_(s) {}
  String(const String &s) : string_(NULL) { *this = s.string_; }
  ~String() { delete[] string_; }

  void Clear() {
    char *s = new char[1];

    s[0] = '\0';
    delete[] string_;
    string_ = s;
  }

  bool IsEmpty() const { return strlen(string_) == 0; }

  const char* Get() const      { return string_; }
  operator const char*() const { return string_; }

  String& operator=(const char *s) {
    if (s == NULL) {
      Clear();
    } else {
      char *string_copy = new char[strlen(s)+1];

      strcpy(string_copy, s);
      string_ = string_copy;
    }
    return *this;
  }
  String& operator=(const String &s) { return operator=(s.Get()); }
  bool operator==(const char *s) const { return strcmp(string_, s) == 0; }
  bool operator!=(const char *s) const { return strcmp(string_, s) != 0; }

  int32_t ToInteger() const   { return atoi(string_); }
  int64_t ToInteger64() const {
    int64_t value = 0;
#if TARGET_OS_WIN32
    sscanf(string_, "%I64d", &value);
#else
    sscanf(string_, "%lld", &value);
#endif
    return value;
  }

 protected:
  const char *string_;
};

inline bool operator<(const String &a, const String &b) {
  return strcmp(a, b) < 0;
}

}  // namespace Diadem

#endif  // DIADEM_WRAPPERS_H_
