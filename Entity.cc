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

#include "Diadem/Entity.h"
#include "Diadem/Layout.h"
#include "Diadem/Native.h"
#include "Diadem/Value.h"

namespace Diadem {

const PropertyName
    kPropName = "name",
    kPropText = "text",
    kPropEnabled = "enabled";

Entity::Entity()
    : parent_(NULL), layout_(NULL), native_(NULL),
      button_callback_(NULL), button_data_(NULL) {
}

Entity::~Entity() {
  for (size_t i = 0; i < children_.size(); ++i)
    delete children_[i];
  delete layout_;
  delete native_;
}

void Entity::InitializeProperties(
    const PropertyMap &properties,
    const Factory &factory) {
  const Array<String> keys = properties.AllKeys();

  for (size_t i = 0; i < keys.size(); ++i)
    SetProperty(keys[i], properties[keys[i]]);
}

String Entity::GetTypeName() const {
  if (native_ != NULL) {
    const String native_type = native_->GetTypeName();

    if (!native_type.IsEmpty())
      return native_type;
  }
  if (layout_ != NULL) {
    const String layout_type = layout_->GetTypeName();

    if (!layout_type.IsEmpty())
      return layout_type;
  }
  return String();
}

const uint32_t kMaxPathLength = 256;

String Entity::GetPath() const {
  char path[kMaxPathLength];

  if (name_.IsEmpty()) {
    const String my_name = GetTypeName();

    if (GetParent() == NULL) {
      snprintf(path, kMaxPathLength, "/%s", my_name.Get());
    } else {
      const String parent_path = (GetParent() == NULL) ?
          String() : GetParent()->GetPath();

      snprintf(
          path, kMaxPathLength, "%s/%s%d",
          parent_path.Get(), my_name.Get(),
          GetParent()->ChildIndexByType(this));
    }
  } else {
    snprintf(path, kMaxPathLength, "\"%s\"", name_.Get());
  }
  return String(path);
}

uint32_t Entity::ChildIndexByType(const Entity *child) const {
  DASSERT(child != NULL);
  const String child_type = child->GetTypeName();
  uint32_t count = 0;

  for (uint32_t i = 0; i < ChildrenCount(); ++i) {
    if (ChildAt(i) == child)
      return count + 1;
    if (ChildAt(i)->GetTypeName() == child_type)
      ++count;
  }
  return 0;
}

void Entity::SetLayout(Layout *layout) {
  layout_ = layout;
  if (layout != NULL)
    layout->SetEntity(this);
}

void Entity::SetNative(Native *native) {
  native_ = native;
  if (native != NULL)
    native->SetEntity(this);
}

Window* Entity::GetWindow() {
  if (parent_ != NULL)
    return parent_->GetWindow();
  return window_;
}

const Window* Entity::GetWindow() const {
  if (parent_ != NULL)
    return parent_->GetWindow();
  return window_;
}

void Entity::FactoryFinalize() {
  for (uint32_t i = 0; i < children_.size(); ++i)
    children_[i]->FactoryFinalize();
  if (layout_ != NULL)
    layout_->Finalize();
  if (native_ != NULL)
    native_->Finalize();
  Finalize();

  // Broadcast initial value in case there are bindings.
  PropertyChanged(kPropValue);
}

void Entity::AddNativeChild(Entity *child) {
  if (child->GetNative() == NULL) {
    for (uint32_t i = 0; i < child->ChildrenCount(); ++i) {
      AddNativeChild(child->ChildAt(i));
    }
  } else {
    AddNative(child->GetNative());
  }
}

ChangeMessenger* Entity::GetChangeMessenger() {
  if (GetParent() != NULL)
    return GetParent()->GetChangeMessenger();
  return NULL;
}

ChangeMessenger const* Entity::GetChangeMessenger() const {
  if (GetParent() != NULL)
    return GetParent()->GetChangeMessenger();
  return NULL;
}

void Entity::AddChild(Entity *child) {
  DASSERT(child != NULL);
  if (child != NULL) {
    children_.push_back(child);
    child->SetParent(this);
    ChildAdded(child);
    AddNativeChild(child);
    if (layout_ != NULL)
      layout_->ChildAdded(child);
  }
}

void Entity::RemoveChild(Entity *child) {
  DASSERT(child->GetParent() == this);
  ChildRemoved(child);
  children_.Remove(child);
  child->SetParent(NULL);
}

void Entity::AddNative(Native *n) {
  if (native_ != NULL)
    native_->AddChild(n);
  else if (parent_ != NULL)
    parent_->AddNative(n);
}

Entity* Entity::FindByName(const char *name) {
  if (name_ == name)
    return this;

  for (size_t i = 0; i < children_.size(); ++i) {
    Entity* result = children_[i]->FindByName(name);

    if (result != NULL)
      return result;
  }
  return NULL;
}

bool Entity::SetProperty(const char *name, const Value &value) {
  if (strcmp(name, kPropName) == 0) {
    SetName(value.Coerce<String>());
    return true;
  }
  if (SetLayoutProperty(name, value))
    return true;
  if (SetNativeProperty(name, value))
    return true;
  return false;
}

Value Entity::GetProperty(const char *name) const {
  if (strcmp(name, kPropName) == 0)
    return GetName();

  Value layout_result = GetLayoutProperty(name);

  if (layout_result.IsValid())
    return layout_result;
  return GetNativeProperty(name);
}

bool Entity::SetLayoutProperty(const char *name, const Value &value) {
  if (layout_ != NULL)
    return layout_->SetProperty(name, value);
  return false;
}

Value Entity::GetLayoutProperty(const char *name) const {
  if (layout_ != NULL)
    return layout_->GetProperty(name);
  return Value();
}

bool Entity::SetNativeProperty(const char *name, const Value &value) {
  if (native_ != NULL)
    return native_->SetProperty(name, value);
  return false;
}

Value Entity::GetNativeProperty(const char *name) const {
  if (native_ != NULL)
    return native_->GetProperty(name);
  return Value();
}

void Entity::PropertyChanged(PropertyName name) const {
  DASSERT((name != NULL) && (name[0] != '\0'));
  if ((name == NULL) || (name[0] == '\0'))
    return;
  if (name_.IsEmpty())
    return;

  const ChangeMessenger* const messenger = GetChangeMessenger();

  if (messenger != NULL)
    messenger->NotifyChange(
        ChangeMessenger::GetPropertyPath(name_, name), GetProperty(name));
}

void Entity::SetText(const char *text) {
  SetProperty(kPropText, Value(text));
}

String Entity::GetText() const {
  const Value text = GetProperty(kPropText);

  return text.IsValid() ? text.Coerce<String>() : String();
}

void Entity::ChildValueChanged(Entity *child) {
  if (layout_ != NULL)
    layout_->ChildValueChanged(child);
  if (native_ != NULL)
    native_->ChildValueChanged(child);
}

void Entity::Clicked(Entity *target) {
  if (button_callback_ != NULL)
    (*button_callback_)(target, button_data_);
  else if (GetParent() != NULL)
    GetParent()->Clicked(target);
}

bool EntityDelegate::SetProperty(PropertyName name, const Value &value) {
  return false;
}

Value EntityDelegate::GetProperty(PropertyName name) const {
  return Value();
}

}  // namespace Diadem
