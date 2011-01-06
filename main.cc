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

#include <gtest/gtest.h>

#include "Diadem/pyadem.h"

extern "C" int NSApplicationMain(int argc, const char *argv[]);
extern "C" void NSApplicationLoad();

int InitPython() {
  static const char *pyadem_name = "pyadem";

  PyImport_AppendInittab(const_cast<char*>(pyadem_name), initpyadem);
  Py_Initialize();

  PyObject *module = PyImport_ImportModule(pyadem_name);

  if (module == NULL)
    return -1;
  return 0;
}

int main(int argc, const char* argv[]) {
  if (InitPython() != 0)
    return -1;

  int result = 0;

  if ((argc > 1) && (strcmp(argv[1], "-test") == 0)) {
    testing::InitGoogleTest(&argc, const_cast<char**>(argv));
    NSApplicationLoad();  // needed for Python tests
    result = RUN_ALL_TESTS();
  } else {
    NSApplicationMain(argc, argv);
  }

  Py_Finalize();
  return result;
}
