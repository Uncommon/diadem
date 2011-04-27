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

#include "Diadem/Native.h"
#include "Diadem/Layout.h"

namespace Diadem {

const PropertyName
    kPropButtonType = "type",
    kPropColumnType = "type",
    kPropData       = "data",
    kPropFile       = "file",
    kPropMax        = "max",
    kPropMin        = "min",
    kPropRowCount   = "rows",
    kPropStyle      = "style",
    kPropTextAlign  = "text-align",
    kPropTicks      = "ticks",
    kPropUISize     = "uisize",
    kPropURL        = "url",
    kPropValue      = "value";

const TypeName
    kTypeNameAppIcon    = "appicon",
    kTypeNameBox        = "box",
    kTypeNameButton     = "button",
    kTypeNameCheck      = "check",
    kTypeNameColumn     = "column",
    kTypeNameCombo      = "combo",
    kTypeNameDate       = "date",
    kTypeNameEdit       = "edit",
    kTypeNameImage      = "image",
    kTypeNameItem       = "item",
    kTypeNameLabel      = "label",
    kTypeNameLink       = "link",
    kTypeNameList       = "list",
    kTypeNamePassword   = "password",
    kTypeNamePath       = "path",
    kTypeNamePopup      = "popup",
    kTypeNameRadio      = "radio",
    kTypeNameRadioGroup = "radiogroup",
    kTypeNameSeparator  = "separator",
    kTypeNameSlider     = "slider",
    kTypeNameTab        = "tab",
    kTypeNameTabs       = "tabs",
    kTypeNameWindow     = "window";

const StringConstant
    kTextAlignLeft   = "left",
    kTextAlignCenter = "center",
    kTextAlignRight  = "right";

const StringConstant
    kUISizeNormal = "normal",
    kUISizeSmall  = "small",
    kUISizeMini   = "mini";

const StringConstant
    kWindowStyleNameClosable    = "close",
    kWindowStyleNameResizable   = "size",
    kWindowStyleNameMinimizable = "min";

const StringConstant
    kButtonTypeNameDefault = "default",
    kButtonTypeNameCancel  = "cancel";

const StringConstant
    kLabelStyleNameHead = "head";

const StringConstant
    kColumnTypeNameText  = "text",
    kColumnTypeNameCheck = "check";

Location Native::GetViewOffset() const {
  Location view_location;

  if (entity_->GetParent() != NULL) {
    Layout *parent_layout = entity_->GetParent()->GetLayout();

    if (parent_layout != NULL)
      view_location = parent_layout->GetViewLocation();
  }
  return view_location;
}

static const uint32_t kStyleCount = 3;

uint32_t Native::ParseWindowStyle(const char *style) {
  const StringConstant style_names[kStyleCount] = {
      kWindowStyleNameClosable,
      kWindowStyleNameResizable,
      kWindowStyleNameMinimizable };
  const uint32_t style_bits[kStyleCount] = {
      kStyleClosable, kStyleResizable, kStyleMinimizable };
  const char *separators = ", ";
  uint32_t result = 0;
  char * const style_copy = strdup(style);
  char *word, *next = style_copy;

  if (style_copy == NULL)
    return 0;
  while (next != NULL) {
    word = strsep(&next, separators);
    for (uint32_t i = 0; i < kStyleCount; ++i) {
      if (strncmp(word, style_names[i], strlen(style_names[i])) == 0) {
        result |= style_bits[i];
        break;
      }
    }
  }
  free(style_copy);
  return result;
}

void RadioGroup::InitializeProperties(
      const PropertyMap &properties,
      const Factory &factory) {
  SetLayout(new Group());
  layout_->SetProperty(kPropDirection, Layout::kLayoutColumn);
  Entity::InitializeProperties(properties, factory);
}

void RadioGroup::Finalize() {
  // Select the first option by default.
  SetSelectedIndex(0);
}

bool RadioGroup::SetProperty(PropertyName name, const Value &value) {
  if (strcmp(name, kPropValue) == 0) {
    SetSelectedIndex(value.Coerce<uint32_t>());
    return true;
  }
  return Entity::SetProperty(name, value);
}

Value RadioGroup::GetProperty(PropertyName name) const {
  if (strcmp(name, kPropPadding) == 0) {
    return Spacing::Union(
        layout_->GetProperty(kPropPadding).Coerce<Spacing>(),
        layout_->GetPlatformMetrics().radio_group_padding);
  }
  if (strcmp(name, kPropValue) == 0) {
    for (uint32_t i = 0; i < ChildrenCount(); ++i) {
      const Value value = ChildAt(i)->GetProperty(kPropValue);

      if (value.IsValid() && (value.Coerce<uint32_t>() != 0))
        return i;
    }
    return Value();
  }
  return Entity::GetProperty(name);
}

void RadioGroup::ChildValueChanged(Entity *child) {
  DASSERT(child != NULL);
  const Value value = child->GetProperty(kPropValue);

  if (value.IsValid() && (value.Coerce<int32_t>() != 0))
    for (uint32_t i = 0; i < ChildrenCount(); ++i)
      if (child == ChildAt(i))
        SetSelectedIndex(i);
}

void RadioGroup::SetSelectedIndex(uint32_t index) {
  for (uint32_t i = 0; i < ChildrenCount(); ++i)
    ChildAt(i)->SetProperty(kPropValue, (i == index) ? 1 : 0);
}

}  // namespace Diadem
