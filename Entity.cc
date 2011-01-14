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
    Entity::kPropName = "name",
    Entity::kPropText = "text",
    Entity::kPropEnabled = "enabled";

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

void Entity::InitializeProperties(const PropertyMap &properties) {
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

void Entity::FactoryFinalize() {
  for (uint32_t i = 0; i < children_.size(); ++i) {
    if (children_[i]->GetNative() != NULL)
      children_[i]->GetNative()->Finalize();
    children_[i]->Finalize();
  }
  if (layout_ != NULL)
    layout_->Finalize();
  if (native_ != NULL)
    native_->Finalize();
  Finalize();
}

void Entity::AddChild(Entity *child) {
  DASSERT(child != NULL);
  if (child != NULL) {
    children_.push_back(child);
    child->SetParent(this);
    ChildAdded(child);
    if (child->GetNative() != NULL)
      AddNative(child->GetNative());
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

Bool Entity::SetProperty(const char *name, const Value &value) {
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

Bool Entity::SetLayoutProperty(const char *name, const Value &value) {
  if (layout_ != NULL)
    return layout_->SetProperty(name, value);
  return false;
}

Value Entity::GetLayoutProperty(const char *name) const {
  if (layout_ != NULL)
    return layout_->GetProperty(name);
  return Value();
}

Bool Entity::SetNativeProperty(const char *name, const Value &value) {
  if (native_ != NULL)
    return native_->SetProperty(name, value);
  return false;
}

Value Entity::GetNativeProperty(const char *name) const {
  if (native_ != NULL)
    return native_->GetProperty(name);
  return Value();
}

void Entity::SetText(const char *text) {
  SetProperty(kPropText, Value(text));
}

String Entity::GetText() const {
  const Value text = GetProperty(kPropText);

  return text.IsValid() ? text.Coerce<String>() : String();
}

void Entity::Clicked(Entity *target) {
  if (button_callback_ != NULL)
    (*button_callback_)(target, button_data_);
  else if (GetParent() != NULL)
    GetParent()->Clicked(target);
}

Bool EntityDelegate::SetProperty(PropertyName name, const Value &value) {
  return false;
}

Value EntityDelegate::GetProperty(PropertyName name) const {
  return Value();
}

}  // namespace Diadem
