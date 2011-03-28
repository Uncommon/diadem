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

#include "Diadem/Binding.h"
#include "Diadem/Native.h"
#include "Diadem/Value.h"

namespace Diadem {

const PropertyName
    kPropProperty  = "prop",
    kPropSource    = "source",
    kPropTransform = "transform";

const TypeName kTypeNameBinding = "bind";

const StringConstant
    kTransformNegate = "not",
    kTransformNotEmpty = "notempty";

bool Binding::SetProperty(PropertyName name, const Value &value) {
  if (strcmp(name, kPropSource) == 0) {
    source_ = value.Coerce<String>();
    return true;
  }
  if (strcmp(name, kPropProperty) == 0) {
    controller_.SetPropertyName(value.Coerce<String>());
    return true;
  }
  if (strcmp(name, kPropTransform) == 0) {
    const String transform = value.Coerce<String>();

    if (transform == kTransformNegate)
      controller_.SetTransformer(new NegateTransform);
    else if (transform == kTransformNotEmpty)
      controller_.SetTransformer(new NotEmptyTransform);
  }
  return Entity::SetProperty(name, value);
}

void Binding::ParentAdded() {
  DASSERT(!source_.IsEmpty());
  controller_.SetEntity(GetParent());
  GetChangeMessenger()->AddObserver(
      ChangeMessenger::GetPropertyPath(source_, kPropValue),
      &controller_);
}

Binding::~Binding() {
  ChangeMessenger* const messenger = GetChangeMessenger();

  if (messenger != NULL)
    messenger->RemoveObserver(&controller_);
}

}  // namespace Diadem
