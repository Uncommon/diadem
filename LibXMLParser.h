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

namespace Diadem {

class Factory;

class LibXMLParser : public Parser {
 public:
  explicit LibXMLParser(Factory &factory) : factory_(factory) {
    factory_.SetParser(this);
  }
  virtual ~LibXMLParser() {
    factory_.SetParser(NULL);
  }

  Entity* LoadEntityFromFile(const char *path) const;
  Entity* LoadEntityFromData(const char *data) const;

 protected:
  Factory &factory_;
};

}  // namespace Diadem
