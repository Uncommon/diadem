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

#ifndef DIADEM_BINDING_H_
#define DIADEM_BINDING_H_

#include "Diadem/ChangeMessenger.h"
#include "Diadem/Entity.h"

namespace Diadem {

extern const PropertyName
    kPropProperty,   // Attribute to be changed in response
    kPropSource,     // Name of the entity being tracked
    kPropTransform;  // Transformation applied to the value

extern const TypeName kTypeNameBinding;

// Listens for changes in one entity and makes corresponding changes in another.
class Binding : public Entity {
 public:
  Binding() {}
  ~Binding();

  virtual bool SetProperty(PropertyName name, const Value &value);

  const EntityController& GetController() const { return controller_; }

 protected:
  String source_;
  EntityController controller_;

  virtual void ParentAdded();
};

}  // namespace Diadem

#endif  // DIADEM_BINDING_H_
