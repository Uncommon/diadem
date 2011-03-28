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

#include "Diadem/Factory.h"
#include "Diadem/LabelGroup.h"
#include "Diadem/Layout.h"
#include "Diadem/Native.h"
#include "Diadem/Value.h"

#define kOSProperty "os"

#if TARGET_OS_MAC
#define kOSName "mac"
#elif TARGET_OS_WIN32
#define kOSName "win"
#else
#define kOSName ""
#endif

namespace Diadem {

void Factory::RegisterBasicClasses() {
  RegisterCreator(
      kTypeNameGroup,
      &Creator<Entity, Entity>::Create,
      &Creator<Group, Layout>::Create,
      NULL);
  RegisterCreator(
      kTypeNameMulti,
      &Creator<Entity, Entity>::Create,
      &Creator<Multipanel, Layout>::Create,
      NULL);
  RegisterCreator(
      kTypeNameSpacer,
      &Creator<Entity, Entity>::Create,
      &Creator<Spacer, Layout>::Create,
      NULL);
  RegisterCreator(
      kTypeNameLabelGroup,
      &Creator<LabelGroup, Entity>::Create,
      NULL, NULL);
  RegisterCreator(
      kTypeNameRadioGroup,
      &Creator<RadioGroup, Entity>::Create,
      NULL, NULL);
}

Entity* Factory::CreateEntity(
      const char *class_name, const PropertyMap &properties) const {
  if (!registry_.Exists(class_name))
    return NULL;

  const CreatorFunctions creators = registry_[String(class_name)];
  Entity* const entity = (*creators.entity_creator)();

  if (entity == NULL)
    return NULL;
  if (creators.layout_creator != NULL) {
    Layout *layout = (*creators.layout_creator)();

    if (layout != NULL) {
      layout->InitializeProperties(properties);
      entity->SetLayout(layout);
    }
  }
  if (creators.native_creator != NULL) {
    Native *native = (*creators.native_creator)();

    if (native != NULL) {
      native->InitializeProperties(properties);
      entity->SetNative(native);
    }
  }
  entity->InitializeProperties(properties, *this);
  return entity;
}

void FactorySession::BeginEntity(
    const char *name, const PropertyMap &properties) {
  if (properties.Exists(kOSProperty) &&
      properties[kOSProperty].Coerce<String>() != kOSName) {
    return;
  }

  Entity* const new_entity = factory_.CreateEntity(name, properties);

  if (new_entity != NULL) {
    Entity* const current_entity = CurrentEntity();

    if (current_entity != NULL)
      current_entity->AddChild(new_entity);
    if (root_ == NULL)
      root_ = new_entity;
  }
  entity_stack_.push(new_entity);
}

void FactorySession::EndEntity() {
  Entity* const current_entity = CurrentEntity();

  if ((current_entity != NULL) && (current_entity->GetParent() == NULL))
    current_entity->FactoryFinalize();
  if (!entity_stack_.empty())
    entity_stack_.pop();
}

}  // namespace Diadem
