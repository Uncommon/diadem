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

#include "Diadem/Value.h"

namespace Diadem {

Value& Value::operator=(const Value &v) {
  if (this != &v) {
    if (v.holder_ == NULL)
      holder_ = NULL;
    else
      holder_ = v.holder_->Copy();
  }
  return *this;
}

String Value::StringFromInt(int i) {
  char c[20] = {};

  snprintf(c, sizeof(c), "%d", i);
  return String(c);
}

}  // namespace Diadem
