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

#ifndef DIADEM_CHANGEMESSENGER_H_
#define DIADEM_CHANGEMESSENGER_H_

#include "Diadem/Base.h"
#include "Diadem/Wrappers.h"

namespace Diadem {

class Entity;
class Value;
class ValueObserver;
class ValueTransformer;

// The ChangeMessenger implements a publish/subscribe model for objects to be
// notified of changes to a named value: the value property of the Entity with
// that name. Every window will have one ChangeMessenger to communicate changes.
class ChangeMessenger : public Base {
 public:
  ChangeMessenger() {}

  // Adds an observer to be notified of changes in a named value. If name is
  // NULL or empty, observer will be notified of all changes.
  void AddObserver(const char *name, ValueObserver *observer);
  // Removes an observer from all notifications.
  void RemoveObserver(ValueObserver *observer);
  // Notifies all appropriate observers that a value has changed.
  void NotifyChange(const char *value_name, const Value &newValue) const;

  // Returns the path used for listening to value changes: "name.property".
  static String GetPropertyPath(
      StringConstant name, PropertyName property);

 protected:
  typedef Set<ValueObserver*> OmniObserverSet;
  typedef MultiMap<String, ValueObserver*> ObserverMap;
  OmniObserverSet omni_observers_;
  ObserverMap observers_;
};

// Modifies a value before it is passed on to an abserver.
class ValueTransformer : public Base {
 public:
  virtual ~ValueTransformer() {}
  virtual Value operator()(const Value &v) = 0;
};

// Abstract class for objects to be notified by the ChangeMessenger.
class ValueObserver : public Base {
 public:
  // Object assumes ownership of the transformer.
  explicit ValueObserver(ValueTransformer *t = NULL) : transformer_(t) {}
  virtual ~ValueObserver() {}

  // Called by ChangeMessenger::NotifyChange.
  // Subclasses should override ObserveImp.
  void Observe(PropertyName name, const Value &v);

  void SetTransformer(ValueTransformer *transformer);

 protected:
  ValueTransformer *transformer_;

  // The named value has changed to a new (maybe transformed) value
  virtual void ObserveImp(PropertyName name, const Value &v) = 0;
};

// An observer that applies a changed value to a specified property of the
// entity it controls.
class EntityController : public ValueObserver {
 public:
  EntityController() : entity_(NULL) {}
  EntityController(
      Entity *entity, PropertyName property, ValueTransformer *t = NULL)
      : ValueObserver(t), entity_(entity), property_(property) {}

  void SetEntity(Entity *entity) { entity_ = entity; }
  void SetPropertyName(PropertyName property) { property_ = property; }
  Entity* GetEntity() const { return entity_; }
  const String& GetPropertyName() const { return property_; }

 protected:
  Entity *entity_;
  String property_;

  virtual void ObserveImp(PropertyName name, const Value &v);
};

// Returns the negation of v as a bool.
class NegateTransform : public ValueTransformer {
 public:
  Value operator()(const Value &v);
};

// Return true if v as a string is not empty, false otherwise.
class NotEmptyTransform : public ValueTransformer {
 public:
  Value operator()(const Value &v);
};

}  // namespace Diadem

#endif  // DIADEM_CHANGEMESSENGER_H_
