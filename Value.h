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

#ifndef DIADEM_VALUE_H_
#define DIADEM_VALUE_H_

#if !DIADEM_PYTHON
class PyObject;
#else
#include <Python/Python.h>
typedef PyObject *PyObjectPtr;
#endif

#if TARGET_OS_WIN32
#include <typeinfo.h>
#else
#include <typeinfo>
using std::type_info;
#endif

#include "Diadem/Wrappers.h"
#include "Diadem/Entity.h"
#include "Diadem/Metrics.h"

namespace Diadem {

class ListDataInterface;

// Generic value holder based roughly on boost::any, but also with type
// conversions using the Coerce method.
class Value {
 public:
  Value() : holder_(NULL) {}
  Value(const Value& value)
      : holder_((value.holder_ == NULL) ? NULL : value.holder_->Copy()) {}

  Value(const char *str) : holder_(new ValueHolder<String>(str)) {}

#define Value_Construct(T) \
  Value(const T &t) : holder_(new ValueHolder<T>(t)) {}

  Value_Construct(bool)
  Value_Construct(int32_t)
  Value_Construct(uint32_t)
  Value_Construct(int64_t)
  Value_Construct(String)
  Value_Construct(double)
  Value_Construct(Size)
  Value_Construct(Spacing)
  Value_Construct(Location)
#if DIADEM_PYTHON
  Value_Construct(PyObjectPtr)
#endif

#undef Value_Construct

  // The macro doesn't work with pointers
  Value(ListDataInterface *data)
      : holder_(new ValueHolder<ListDataInterface*>(data)) {}

  template <class T>
  Value& operator=(const T &t) {
    delete holder_;
    holder_ = new ValueHolder<T>(t);
    return *this;
  }

  ~Value() { delete holder_; }

  bool IsValid() const { return holder_ != NULL; }

  template <class T>
  bool IsValueType() const
    { return (holder_ != NULL) && (holder_->Type() == typeid(T)); }

  void Clear() {
    delete holder_;
    holder_ = NULL;
  }

  // Used to call different overloads of ValueHolder::Coerce
  template <class T>
  struct type {};

  // This is the method to use to get the value out, whether or not you think
  // it will require any conversion.
  template <class T>
  T Coerce() const {
    if (holder_ == NULL)
      return T();
    ValueHolder<T> *t_holder = dynamic_cast<ValueHolder<T>*>(holder_);
    if (t_holder != NULL)
      return t_holder->data;
    return holder_->Coerce(type<T>());
  }

  Value& operator=(const Value& v);

  Value& operator=(const char *str)
    { return operator=(String(str)); }

  static String StringFromInt(int i);

 protected:
  class ValueHolderBase {
   public:
    virtual ~ValueHolderBase() {}
    virtual const type_info& Type() = 0;
    virtual ValueHolderBase* Copy() = 0;

    // Each type variant must be declared explicitly because method templates
    // cannot be virtual.
    virtual int32_t Coerce(const type<int32_t>&) const = 0;
    virtual uint32_t Coerce(const type<uint32_t>&) const = 0;
    virtual int64_t Coerce(const type<int64_t>&) const = 0;
    virtual bool Coerce(const type<bool>&) const = 0;
    virtual String Coerce(const type<String>&) const = 0;
    virtual double Coerce(const type<double>&) const = 0;
    virtual Spacing Coerce(const type<Spacing>&) const = 0;
#if DIADEM_PYTHON
    virtual PyObject* Coerce(const type<PyObject*>&) const = 0;
#endif

    // This shouldn't ever get called, but it is needed in order for
    // Value::Coerce to compile.
    template <class T>
    T Coerce(const type<T>&) const {
      DASSERT(false);
      return T();
    }
  };

  template <class T>
  class ValueHolder : public ValueHolderBase {
   public:
    ValueHolder(const T& value) : data(value) {}

    const type_info& Type()       { return typeid(T); }
    ValueHolderBase* Copy()       { return new ValueHolder<T>(data); }

    // For types where simple casts are not enough, these are overridden
    // by specializations below.
    int32_t Coerce(const type<int32_t>&) const { return (int32_t)data; }
    uint32_t Coerce(const type<uint32_t>&) const { return (uint32_t)data; }
    int64_t Coerce(const type<int64_t>&) const { return (int64_t)data; }
    bool    Coerce(const type<bool>&) const    { return data ? true : false; }
    String  Coerce(const type<String>&) const
        { return StringFromInt((int)data); }
    double  Coerce(const type<double>&) const  { return (double)data; }
    Spacing Coerce(const type<Spacing>&) const { return Spacing(); }
    Location Coerce(const type<Location>&) const { return Location(); }
#if DIADEM_PYTHON
    PyObject* Coerce(const type<PyObjectPtr>&) const { return NULL; }
#endif

    const T data;
  };

  ValueHolderBase *holder_;
};

// TODO(catmull): clean this up using something like tr1::is_pod

template<> inline int32_t Value::ValueHolder<String>::Coerce(
    const Value::type<int32_t>&) const { return data.ToInteger(); }
template<> inline uint32_t Value::ValueHolder<String>::Coerce(
    const Value::type<uint32_t>&) const { return data.ToInteger(); }
template<> inline int64_t Value::ValueHolder<String>::Coerce(
    const Value::type<int64_t>&) const { return data.ToInteger64(); }
template<> inline bool Value::ValueHolder<String>::Coerce(
    const Value::type<bool>&) const
  { return Coerce(Value::type<int32_t>()) ? true : false; }
template<> inline String Value::ValueHolder<String>::Coerce(
    const Value::type<String>&) const { return data; }
template<> inline double Value::ValueHolder<String>::Coerce(
    const Value::type<double>&) const { return data.ToDouble(); }

#define ValueReturnData(T) \
  template<> inline T Value::ValueHolder<T>::Coerce( \
      const Value::type<T>&) const { return data; }

ValueReturnData(bool)
ValueReturnData(Spacing)
ValueReturnData(Location)

#define CoerceToPODZero(T) \
  template<> inline int32_t Value::ValueHolder<T>::Coerce( \
      const Value::type<int32_t>&) const { return 0; } \
  template<> inline uint32_t Value::ValueHolder<T>::Coerce( \
      const Value::type<uint32_t>&) const { return 0; } \
  template<> inline int64_t Value::ValueHolder<T>::Coerce( \
      const Value::type<int64_t>&) const { return 0; } \
  template<> inline bool Value::ValueHolder<T>::Coerce( \
      const Value::type<bool>&) const { return false; } \
  template<> inline String Value::ValueHolder<T>::Coerce( \
      const Value::type<String>&) const { return String(); } \
  template<> inline double Value::ValueHolder<T>::Coerce( \
      const Value::type<double>&) const { return 0.0f; }

CoerceToPODZero(Size)
CoerceToPODZero(Spacing)
CoerceToPODZero(Location)
CoerceToPODZero(ListDataInterface*)

#undef CoerceToPODZero
#undef ValueReturnData

#if DIADEM_PYTHON
template<> inline int32_t Value::ValueHolder<PyObjectPtr>::Coerce(
    const Value::type<int32_t>&) const { return PyInt_AsLong(data); }
template<> inline uint32_t Value::ValueHolder<PyObjectPtr>::Coerce(
    const Value::type<uint32_t>&) const { return PyLong_AsUnsignedLong(data); }
// there is no PyInt_AsLongLong
template<> inline bool Value::ValueHolder<PyObjectPtr>::Coerce(
    const Value::type<bool>&) const { return PyObject_IsTrue(data); }
template<> inline String Value::ValueHolder<PyObjectPtr>::Coerce(
    const Value::type<String>&) const { return PyString_AsString(data); }
template<> inline double Value::ValueHolder<PyObjectPtr>::Coerce(
    const Value::type<double>&) const { return PyFloat_AsDouble(data); }
// Spacing and Location will not be needed

template<> inline PyObjectPtr Value::ValueHolder<int32_t>::Coerce(
    const Value::type<PyObjectPtr>&) const { return PyInt_FromLong(data); }
template<> inline PyObjectPtr Value::ValueHolder<bool>::Coerce(
    const Value::type<PyObjectPtr>&) const { return PyBool_FromLong(data); }
template<> inline PyObjectPtr Value::ValueHolder<String>::Coerce(
    const Value::type<PyObjectPtr>&) const { return PyString_FromString(data); }
template<> inline PyObjectPtr Value::ValueHolder<double>::Coerce(
    const Value::type<PyObjectPtr>&) const { return PyFloat_FromDouble(data); }
#else
template<> inline double Value::ValueHolder<PyObjectPtr>::Coerce(
    const Value::type<double>&) const { return 0.0; }
#endif

}  // namespace Diadem

#endif  // DIADEM_VALUE_H_
