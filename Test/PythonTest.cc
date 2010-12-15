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
#include "Diadem/Entity.h"
#include "Diadem/Layout.h"
#include "Diadem/Native.h"
#include "Diadem/Value.h"

class PythonTest : public testing::Test {
 public:
  void SetUp() {
    globals_ = PyEval_GetBuiltins();
    locals_ = PyDict_New();
    PyDict_SetItemString(locals_, "pyadem", PyImport_AddModule("pyadem"));
  }
  void TearDown() {
    Py_DECREF(locals_);
  }

  PyObject *globals_, *locals_;
};

TEST_F(PythonTest, ValueCoersions) {
  const Diadem::Value v(1);
  PyObject *o = v.Coerce<PyObject*>();

  EXPECT_FALSE(o == NULL);
  if (o != NULL) {
    EXPECT_EQ(1, PyInt_AsLong(o));
    Py_DECREF(o);
  }

  o = PyInt_FromLong(2);

  int i = Diadem::Value(o).Coerce<int32_t>();

  EXPECT_EQ(2, i);
  Py_DECREF(o);
}

TEST_F(PythonTest, Window) {
  PyObject *result = PyRun_String(
      "window = pyadem.Window(data=\"<window text='spiny'/>\")\n"
      "did_show = window.ShowModeless()",
      Py_file_input, globals_, locals_);
  PyObject *error = PyErr_Occurred();

  if (error != NULL) {
    PyErr_Print();
    PyErr_Clear();
    FAIL();
  }
  ASSERT_FALSE(result == NULL);

  PyObject *window = PyDict_GetItemString(locals_, "window");

  ASSERT_FALSE(window == NULL);
  EXPECT_TRUE(PyObject_TypeCheck(window, &EntityType));
  EXPECT_TRUE(PyObject_TypeCheck(window, &WindowType));

  PyObject *did_show = PyDict_GetItemString(locals_, "did_show");

  ASSERT_FALSE(did_show == NULL);
  EXPECT_TRUE(PyObject_IsTrue(did_show));
}

TEST_F(PythonTest, GetSetName) {
  PyObject *locals = PyModule_GetDict(PyImport_AddModule("pyadem"));
  PyObject *window = PyRun_String(
      "Window(data=\"<window name='spiny'/>\")",
      Py_eval_input, globals_, locals);
  PyObject *error = PyErr_Occurred();

  if (error != NULL) {
    PyErr_Print();
    PyErr_Clear();
    FAIL();
  }
  ASSERT_FALSE(window == NULL);
  ASSERT_TRUE(PyObject_HasAttrString(window, "name"));

  PyObject *name = PyObject_GetAttrString(window, "name");

  ASSERT_FALSE(name == NULL);
  ASSERT_TRUE(PyString_Check(name));
  EXPECT_STREQ("spiny", PyString_AsString(name));
  Py_DECREF(name);

  PyademEntity *py_entity = (PyademEntity*)window;

  EXPECT_STREQ("spiny", py_entity->object->GetName());
  py_entity->object->SetName("norman");
  name = PyObject_GetAttrString(window, "name");
  ASSERT_FALSE(name == NULL);
  ASSERT_TRUE(PyString_Check(name));
  EXPECT_STREQ("norman", PyString_AsString(name));
  Py_DECREF(name);

  PyObject *new_name = PyString_FromString("dinsdale");
  PyObject_SetAttrString(window, "name", new_name);
  Py_DECREF(new_name);
  EXPECT_STREQ("dinsdale", py_entity->object->GetName());

  Py_DECREF(window);
}

TEST_F(PythonTest, Properties) {
  PyObject *result = PyRun_String(
      "window = pyadem.Window(data=\"<window text='spiny'/>\")\n"
      "text = window.GetProperty(\"text\")\n"
      "window.SetProperty(\"text\", \"Norman\")",
      Py_file_input, globals_, locals_);
  PyObject *error = PyErr_Occurred();

  if (error != NULL) {
    PyErr_Print();
    PyErr_Clear();
    FAIL();
  }
  ASSERT_FALSE(result == NULL);

  PyObject *window = PyDict_GetItemString(locals_, "window");

  ASSERT_FALSE(window == NULL);
  ASSERT_TRUE(PyObject_TypeCheck(window, &EntityType));

  PyObject *text = PyDict_GetItemString(locals_, "text");

  ASSERT_FALSE(text == NULL);
  ASSERT_TRUE(PyString_Check(text));
  EXPECT_STREQ("spiny", PyString_AsString(text));

  Diadem::Entity *window_entity = ((PyademEntity*)window)->object;

  EXPECT_STREQ("Norman",
      window_entity->GetProperty("text").Coerce<Diadem::String>().Get());
}

TEST_F(PythonTest, FindByName) {
  PyObject *result = PyRun_String(
      "window = pyadem.Window(data=\""
        "<window><button name='A'/><button name='B'/></window>\")\n"
      "buttonA = window.FindByName(\"A\")\n"
      "buttonB = window.FindByName(\"B\")\n",
      Py_file_input, globals_, locals_);
  PyObject *error = PyErr_Occurred();

  if (error != NULL) {
    PyErr_Print();
    PyErr_Clear();
    FAIL();
  }
  ASSERT_FALSE(result == NULL);

  PyObject *buttonA = PyDict_GetItemString(locals_, "buttonA");
  PyObject *buttonB = PyDict_GetItemString(locals_, "buttonB");

  ASSERT_FALSE(buttonA == NULL);
  ASSERT_FALSE(buttonB == NULL);
  ASSERT_TRUE(PyObject_TypeCheck(buttonA, &EntityType));
  ASSERT_TRUE(PyObject_TypeCheck(buttonB, &EntityType));

  Diadem::Entity *entityA = ((PyademEntity*)buttonA)->object;
  Diadem::Entity *entityB = ((PyademEntity*)buttonB)->object;

  EXPECT_STREQ("A", entityA->GetName());
  EXPECT_STREQ("B", entityB->GetName());
}

TEST_F(PythonTest, ButtonCallback) {
  PyObject *result = PyRun_String(
      "window = pyadem.Window(data=\""
        "<window><button name='A' text='OK'/></window>\")\n"
      "window.context = False\n"
      "def callback(win, button):\n"
      "  win.context = True\n"
      "window.button_callback = callback\n",
      Py_file_input, globals_, locals_);
  PyObject *error = PyErr_Occurred();

  if (error != NULL) {
    PyErr_Print();
    PyErr_Clear();
    FAIL();
  }
  ASSERT_FALSE(result == NULL);

  PyademEntity *window = (PyademEntity*)PyDict_GetItemString(locals_, "window");

  ASSERT_FALSE(window == NULL);
  ASSERT_TRUE(PyObject_TypeCheck(window, &EntityType));
  ASSERT_FALSE(window->context == NULL);
  EXPECT_FALSE(PyObject_IsTrue(window->context));

  Diadem::Entity *button = ((PyademEntity*)window)->object->FindByName("A");

  ASSERT_FALSE(button == NULL);
  button->Clicked();
  EXPECT_TRUE(PyObject_IsTrue(window->context));
}

TEST_F(PythonTest, LoadFromFile) {
  PyObject *result = PyRun_String(
      "window = pyadem.Window(path='test.dem')",
      Py_file_input, globals_, locals_);
  PyObject *error = PyErr_Occurred();

  if (error != NULL) {
    PyErr_Print();
    PyErr_Clear();
    FAIL();
  }
  ASSERT_FALSE(result == NULL);

  PyademEntity *window = reinterpret_cast<PyademEntity*>(
      PyDict_GetItemString(locals_, "window"));

  ASSERT_FALSE(window == NULL);
  ASSERT_TRUE(PyObject_TypeCheck(window, &WindowType));
}

TEST_F(PythonTest, PropertyConstants) {
  PyObject *result = PyRun_String(
      "text = pyadem.PROP_TEXT\n"
      "enabled = pyadem.PROP_ENABLED\n"
      "visible = pyadem.PROP_VISIBLE\n"
      "in_layout = pyadem.PROP_IN_LAYOUT\n"
      "url = pyadem.PROP_URL\n",
      Py_file_input, globals_, locals_);
  PyObject *error = PyErr_Occurred();

  if (error != NULL) {
    PyErr_Print();
    PyErr_Clear();
    FAIL();
  }
  ASSERT_FALSE(result == NULL);
  PyObject *text = PyDict_GetItemString(locals_, "text");
  PyObject *enabled = PyDict_GetItemString(locals_, "enabled");
  PyObject *visible = PyDict_GetItemString(locals_, "visible");
  PyObject *in_layout = PyDict_GetItemString(locals_, "in_layout");
  PyObject *url = PyDict_GetItemString(locals_, "url");

  EXPECT_STREQ(Diadem::Entity::kPropText, PyString_AsString(text));
  EXPECT_STREQ(Diadem::Entity::kPropEnabled, PyString_AsString(enabled));
  EXPECT_STREQ(Diadem::kPropVisible, PyString_AsString(visible));
  EXPECT_STREQ(Diadem::kPropInLayout, PyString_AsString(in_layout));
  EXPECT_STREQ(Diadem::kPropURL, PyString_AsString(url));
}

#define RUN_INTERACTING_TESTS 0

#if RUN_INTERACTING_TESTS  // TODO(catmull): automate the button clicking
TEST_F(PythonTest, MessagePlain) {
  PyObject *result = PyRun_String(
      "button = pyadem.ShowMessage('Something happened!')",
      Py_file_input, globals_, locals_);
  PyObject *error = PyErr_Occurred();

  if (error != NULL) {
    PyErr_Print();
    PyErr_Clear();
    FAIL();
  }
  ASSERT_FALSE(result == NULL);

  PyObject *button = PyDict_GetItemString(locals_, "button");

  ASSERT_FALSE(button == NULL);
  ASSERT_TRUE(PyString_Check(button));
  EXPECT_STREQ("accept", PyString_AsString(button));
}

TEST_F(PythonTest, MessageCancel) {
  PyObject *result = PyRun_String(
      "button = pyadem.ShowMessage('Click Cancel for me.', cancel=True)",
      Py_file_input, globals_, locals_);
  PyObject *error = PyErr_Occurred();

  if (error != NULL) {
    PyErr_Print();
    PyErr_Clear();
    FAIL();
  }
  ASSERT_FALSE(result == NULL);

  PyObject *button = PyDict_GetItemString(locals_, "button");

  ASSERT_FALSE(button == NULL);
  ASSERT_TRUE(PyString_Check(button));
  EXPECT_STREQ("cancel", PyString_AsString(button));
}

TEST_F(PythonTest, MessageSuppress) {
  PyObject *result = PyRun_String(
      "msg_result = pyadem.ShowMessage('Somebody stop me!', suppress='Stop')",
      Py_file_input, globals_, locals_);
  PyObject *error = PyErr_Occurred();

  if (error != NULL) {
    PyErr_Print();
    PyErr_Clear();
    FAIL();
  }
  ASSERT_FALSE(result == NULL);

  PyObject *msg_result = PyDict_GetItemString(locals_, "msg_result");

  ASSERT_FALSE(msg_result == NULL);
  ASSERT_TRUE(PyTuple_Check(msg_result));

  PyObject *button = PyTuple_GetItem(msg_result, 0);
  PyObject *suppressed = PyTuple_GetItem(msg_result, 1);

  ASSERT_FALSE(button == NULL);
  EXPECT_STREQ("accept", PyString_AsString(button));
  ASSERT_FALSE(suppressed == NULL);
  EXPECT_TRUE(PyObject_IsTrue(suppressed));
}
#endif
