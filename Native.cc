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
    kPropFile       = "file",
    kPropStyle      = "style",
    kPropTextAlign  = "text-align",
    kPropUISize     = "uisize",
    kPropURL        = "url",
    kPropValue      = "value";

const TypeName
    kTypeNameWindow     = "window",
    kTypeNameBox        = "box",
    kTypeNameButton     = "button",
    kTypeNameCheck      = "check",
    kTypeNameLabel      = "label",
    kTypeNameLink       = "link",
    kTypeNameEdit       = "edit",
    kTypeNamePassword   = "password",
    kTypeNamePath       = "path",
    kTypeNameSeparator  = "separator",
    kTypeNameImage      = "image",
    kTypeNamePopup      = "popup",
    kTypeNameItem       = "item",
    kTypeNameCombo      = "combo",
    kTypeNameDate       = "date",
    kTypeNameList       = "list",
    kTypeNameTabs       = "tabs",
    kTypeNameTab        = "tab";

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
    kLabelStyleNameHead = "head";

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

}  // namespace Diadem
