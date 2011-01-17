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
    kPropTextAlign  = "text-align",
    kPropUISize     = "uisize",
    kPropURL        = "url";

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

Location Native::GetViewOffset() const {
  Location view_location;

  if (entity_->GetParent() != NULL) {
    Layout *parent_layout = entity_->GetParent()->GetLayout();

    if (parent_layout != NULL)
      view_location = parent_layout->GetViewLocation();
  }
  return view_location;
}

}  // namespace Diadem
