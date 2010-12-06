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

PropertyName
    kPropButtonType = "type",
    kPropFile       = "file",
    kPropTextAlign  = "text-align",
    kPropUISize     = "uisize",
    kPropURL        = "url";

Location Native::GetViewOffset() const {
  Location viewLocation;

  if (entity_->GetParent() != NULL) {
    Layout *parentLayout = entity_->GetParent()->GetLayout();

    if (parentLayout != NULL)
      viewLocation = parentLayout->GetViewLocation();
  }
  return viewLocation;
}

}  // namespace Diadem
