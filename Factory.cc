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
      "group",
      &Creator<Entity, Entity>::Create,
      &Creator<Group, Layout>::Create,
      NULL);
  RegisterCreator(
      "spacer",
      &Creator<Entity, Entity>::Create,
      &Creator<Spacer, Layout>::Create,
      NULL);
}

Entity* Factory::CreateEntity(
      const char *className, const PropertyMap &properties) const {
  if (!registry_.Exists(className))
    return NULL;

  const CreatorFunctions creators = registry_[String(className)];
  Entity* const entity = (*creators.entityCreator)();

  if (entity == NULL)
    return NULL;
  if (creators.layoutCreator != NULL) {
    Layout *layout = (*creators.layoutCreator)();

    if (layout != NULL) {
      layout->InitializeProperties(properties);
      entity->SetLayout(layout);
    }
  }
  if (creators.nativeCreator != NULL) {
    Native *native = (*creators.nativeCreator)();

    if (native != NULL) {
      native->InitializeProperties(properties);
      entity->SetNative(native);
    }
  }
  entity->InitializeProperties(properties);
  return entity;
}

void FactorySession::BeginEntity(
    const char *name, const PropertyMap &properties) {
  if (properties.Exists(kOSProperty) &&
      properties[kOSProperty].Coerce<String>() != kOSName) {
    return;
  }

  Entity* const newEntity = factory_.CreateEntity(name, properties);

  if (newEntity != NULL) {
    Entity* const currentEntity = CurrentEntity();

    if (currentEntity != NULL)
      currentEntity->AddChild(newEntity);
    if (root_ == NULL)
      root_ = newEntity;
  }
  entityStack_.push(newEntity);
}

void FactorySession::EndEntity() {
  Entity* const currentEntity = CurrentEntity();

  if ((currentEntity != NULL) && (currentEntity->GetParent() == NULL))
    currentEntity->FactoryFinalize();
  if (!entityStack_.empty())
    entityStack_.pop();
}

}  // namespace Diadem
