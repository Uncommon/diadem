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

#include "Diadem/ChangeMessenger.h"
#include "Diadem/Value.h"

namespace Diadem {

void ChangeMessenger::AddObserver(const char *name, ValueObserver *observer) {
  if ((name == NULL) || (name[0] == '\0')) {
    omni_observers_.insert(observer);
  } else {
    if (!observers_.exists(name, observer))
      observers_.insert(name, observer);
  }
}

void ChangeMessenger::RemoveObserver(ValueObserver *observer) {
  omni_observers_.erase(observer);
  observers_.erase_value(observer);
}

void ChangeMessenger::NotifyChange(
    const char *value_name, const Value &newValue) const {
  for (OmniObserverSet::const_iterator i = omni_observers_.begin();
       i != omni_observers_.end(); ++i)
    (*i)->Observe(value_name, newValue);
  for (ObserverMap::const_iterator i = observers_.begin();
       i != observers_.end(); ++i)
    if (i->first == value_name)
      i->second->Observe(value_name, newValue);
}

String ChangeMessenger::GetPropertyPath(
    StringConstant name, PropertyName property) {
  if ((name == NULL) || (strlen(name) == 0))
    return String();

  // 1 for period, 1 for terminator
  const uint32_t length = strlen(name) + 1 + strlen(property) + 1;
  char *path = new char[length];

  snprintf(path, length, "%s.%s", name, property);
  return String(path, String::kAdoptBuffer);
}

void ValueObserver::Observe(PropertyName name, const Value &v) {
  ObserveImp(name, (transformer_ == NULL) ? v : (*transformer_)(v));
}

void ValueObserver::SetTransformer(ValueTransformer *transformer) {
  delete transformer_;
  transformer_ = transformer;
}

void EntityController::ObserveImp(PropertyName name, const Value &v) {
  entity_->SetProperty(property_, v);
}

Value NegateTransform::operator()(const Value &v) {
  return !v.Coerce<bool>();
}

Value NotEmptyTransform::operator()(const Value &v) {
  return !v.Coerce<String>().IsEmpty();
}

}  // namespace Diadem
