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

#ifndef DIADEM_PYTHON
#include "Diadem.pch"
#endif

#include "Diadem/pyadem.h"

#include <Python/structmember.h>

#include "Diadem/Factory.h"
#include "Diadem/LibXMLParser.h"
#include "Diadem/Native.h"
#include "Diadem/Value.h"

#if TARGET_OS_MAC
#include "Diadem/NativeCocoa.h"
#define DIADEM_PLATFORM Diadem::Cocoa
#endif

// Stack-based class for acquiring Python's Global Interpreter Lock.
// This is needed in UI event handlers that call Python callbacks. Otherwise
// PyObject_Call can crash because the thread state is unset.
class GILState {
 public:
  GILState() : state_(PyGILState_Ensure()) {}
  ~GILState() {
    PyGILState_Release(state_);
  }
 protected:
  PyGILState_STATE state_;
};

// Button names returned from ShowMessage
Diadem::StringConstant
    kButtonNameAccept = "accept",
    kButtonNameCancel = "cancel",
    kButtonNameOther  = "other";

Diadem::StringConstant ButtonNameFromButtonType(Diadem::ButtonType button) {
  switch (button) {
    case Diadem::kAcceptButton:
      return kButtonNameAccept;
    case Diadem::kOtherButton:
      return kButtonNameOther;
    case Diadem::kCancelButton:
    default:
      return kButtonNameCancel;
  }
}

class PyListData : public Diadem::ListDataInterface {
 public:
  explicit PyListData(PyObject *data) : data_(data) {
    Py_INCREF(data_);
  }
  ~PyListData() {
    Py_DECREF(data_);
  }

  virtual Diadem::String GetCellText(size_t row, const char *column) const {
    GILState gil;
    PyObject *result = PyObject_CallMethodObjArgs(
        data_, PyString_FromString("GetCellText"),
        PyLong_FromUnsignedLong(row), PyString_FromString(column), NULL);

    if ((result == NULL) && PyErr_Occurred()) {
      PyErr_Print();
      PyErr_Clear();
    }
    if ((result == NULL) || !PyString_Check(result))
      return Diadem::String();
    return PyString_AsString(result);
  }
  virtual void SetRowChecked(size_t row, bool check) {
    GILState gil;
    PyObject_CallMethodObjArgs(
        data_, PyString_FromString("SetRowChecked"),
        PyLong_FromUnsignedLong(row), PyBool_FromLong(check), NULL);
  }
  virtual bool GetRowChecked(size_t row) const {
    GILState gil;
    PyObject *result = PyObject_CallMethodObjArgs(
        data_, PyString_FromString("GetRowChecked"),
        PyLong_FromUnsignedLong(row), NULL);

    if (result == NULL)
      return false;
    return PyObject_IsTrue(result);
  }
  virtual void ListDeleted() {
    delete this;
  }

 protected:
  PyObject *data_;
};

// Wraps an Entity in a Python object.
static PyademEntity* WrapEntity(Diadem::Entity *entity) {
  PyademEntity *result = PyObject_New(PyademEntity, &EntityType);

  if (result == NULL)
    return NULL;
  result->object = entity;
  result->button_callback = NULL;
  result->context = NULL;
  return result;
}

// Diadem button callback that calls through to the Python window's callback
static void ButtonCallback(Diadem::Entity *button, PyademEntity *self) {
  if ((self->button_callback != NULL) &&
      (PyCallable_Check(self->button_callback))) {
    GILState gil;
    PyademEntity *button_object = WrapEntity(button);
    PyObject *args = PyTuple_Pack(2, self, button_object);

    PyObject_CallObject(self->button_callback, args);
    Py_DECREF(args);
    Py_DECREF(button_object);
    if (PyErr_Occurred()) {
      // If an exception occurred for a modal window, then end the modal loop
      // and propagate the exception to the ShowModal caller
      Diadem::Window *window = button->GetWindow();

      if (!window->EndModal()) {
        // If the window is not modal, then there is no Python caller to handle
        // the exception, so just log it
        PyErr_Print();
        PyErr_Clear();
      }
    }
  }
}

// Diadem close callback that calls through to the Python window's callback
static bool WindowCloseCallback(Diadem::Window *window, PyademWindow *self) {
  if ((self->close_callback == NULL) || !PyCallable_Check(self->close_callback))
    return true;

  GILState gil;
  PyObject *args = PyTuple_Pack(1, self);
  PyObject *result = PyObject_CallObject(self->close_callback, args);

  Py_DECREF(args);
  if (result == NULL)
    return true;

  const bool should_close = PyObject_IsTrue(result);

  Py_DECREF(result);
  return should_close;
}

// Returns the path to the named file in the default resources directory.
// TODO(catmull): This is Mac-specific, and we'll need a Windows equivalent
static char* GetResourcePath(const char *path) {
  CFStringRef name = CFStringCreateWithCString(
      kCFAllocatorDefault, path, kCFStringEncodingUTF8);

  if (name == NULL) {
    PyErr_SetString(PyExc_RuntimeError, "bad name");
    return NULL;
  }

  CFBundleRef bundle = CFBundleGetMainBundle();
  CFURLRef url = CFBundleCopyResourceURL(bundle, name, NULL, NULL);

  CFRelease(name);
  if (url == NULL) {
    PyErr_SetString(PyExc_IOError, "resource file not found");
    return NULL;
  }

  CFStringRef path_string = CFURLCopyPath(url);

  CFRelease(url);
  if (path_string == NULL) {
    PyErr_SetString(PyExc_RuntimeError, "can't make path string");
    return NULL;
  }

  const CFIndex path_length =
      CFStringGetMaximumSizeOfFileSystemRepresentation(path_string);
  char *result = new char[path_length];
  bool converted =
      CFStringGetFileSystemRepresentation(path_string, result, path_length);

  CFRelease(path_string);
  if (!converted) {
    PyErr_SetString(PyExc_RuntimeError, "failed to copy path string");
    delete[] result;
    return NULL;
  }
  return result;
}

// TODO(catmull): This is platform-specific.
bool IsAbsolutePath(const char *path) {
  return path[0] == '/';
}

// Entity constructor. Either the path or the data parameter may be given.
// TODO(catmull): This should be moved to the Window class, since Entities
// can't be constructed directly in Python.
static int Entity_init(PyademEntity *self, PyObject *args, PyObject *keywords) {
  PyObject *path = NULL, *data = NULL;
  static const char *keys[] = { "path", "data" };

  self->object = NULL;
  self->button_callback = Py_None;
  self->context = NULL;
  if (!PyArg_ParseTupleAndKeywords(
      args, keywords, "|SS", const_cast<char**>(keys), &path, &data))
    return -1;
  if ((path == NULL) && (data == NULL))
    return -1;

  Diadem::Factory factory;
  Diadem::LibXMLParser parser(factory);

  DIADEM_PLATFORM::SetUpFactory(&factory);

  if (path != NULL) {
    char *cpath = PyString_AsString(path);

    if ((cpath == NULL) || (cpath[0] == '\0'))
      return -1;

    if (IsAbsolutePath(cpath)) {
      self->object = parser.LoadEntityFromFile(cpath);
    } else {
      cpath = GetResourcePath(cpath);
      if (cpath == NULL)
        return -1;
      self->object = parser.LoadEntityFromFile(cpath);
      delete[] cpath;
    }
  } else {
    self->object = parser.LoadEntityFromData(PyString_AsString(data));
  }
  if (self->object == NULL) {
    PyErr_SetString(PyExc_RuntimeError, "failed to load window data");
    return -1;
  } else {
    self->object->SetButtonCallback(
        (Diadem::Entity::ButtonCallback)ButtonCallback, self);
    return 0;
  }
}

// Entity destructor.
static void Entity_dealloc(PyademEntity *self) {
  if ((self->object != NULL) && (self->object->GetParent() == NULL)) {
    delete self->object;
    self->object = NULL;
  }
  Py_XDECREF(self->button_callback);
  Py_XDECREF(self->context);
  self->ob_type->tp_free(reinterpret_cast<PyObject*>(self));
}

// Getter for the 'name' attribute.
static PyObject* Entity_getName(PyademEntity *self, void *closure) {
  if ((self == NULL) || (self->object == NULL))
    return NULL;
  return PyString_FromString(self->object->GetName());
}

// Getter for the 'name' attribute.
static int Entity_setName(PyademEntity *self, PyObject *value, void *closure) {
  if ((self == NULL) || (self->object == NULL))
    return -1;
  self->object->SetName(Diadem::String(PyString_AsString(value)));
  return 0;
}

static PyGetSetDef Entity_getsetters[] = {
    { const_cast<char*>("name"), (getter)Entity_getName, (setter)Entity_setName,
      const_cast<char*>("name"), NULL },
    { NULL },
    };

// Helper used in Entity_getProperty and Entity_getPropertyByName
static PyObject* GetProperty(Diadem::Entity *entity, PyObject *name) {
  const Diadem::Value value = entity->GetProperty(PyString_AsString(name));

  if (!value.IsValid()) {
    PyErr_SetString(PyExc_KeyError, "property not found");
    return NULL;
  }

  // Sometimes PyString_AsString sets the error indicator even when it succeeds.
  PyErr_Clear();
  return value.Coerce<PyObject*>();
}

// Helper used in Entity_setProperty and Entity_setPropertyByName
static PyObject* SetProperty(
    Diadem::Entity *entity, PyObject *name, PyObject *value) {
  if (strcmp(PyString_AsString(name), Diadem::kPropData) == 0) {
    // Special case: data is a callback object, and must be wrapped
    PyListData *data = new PyListData(value);

    if (!entity->SetProperty(
        Diadem::kPropData, static_cast<Diadem::ListDataInterface*>(data))) {
      PyErr_SetString(PyExc_KeyError, "property not found");
      delete data;
      return NULL;
    }
  } else if (!entity->SetProperty(PyString_AsString(name), value)) {
    PyErr_SetString(PyExc_KeyError, "property not found");
    return NULL;
  }
  PyErr_Clear();
  Py_RETURN_NONE;
}

// Helper used in Entity_get/setPropertyByName
static Diadem::Entity* GetEntityByName(PyademEntity *self, PyObject *name) {
  const char *name_chars = PyString_AsString(name);

  if (name_chars == NULL)
    return NULL;

  Diadem::Entity* const entity = self->object->FindByName(name_chars);

  if (entity == NULL) {
    PyErr_SetString(PyExc_KeyError, "entity not found");
    return NULL;
  }
  return entity;
}

static PyObject* Entity_getProperty(PyademEntity *self, PyObject *name) {
  if (self->object == NULL)
    return NULL;

  return GetProperty(self->object, name);
}

static PyObject* Entity_setProperty(PyademEntity *self, PyObject *args) {
  if (self->object == NULL)
    return NULL;

  PyObject *name, *value;

  if (!PyArg_ParseTuple(args, "SO", &name, &value))
    return NULL;
  if (PyString_Size(name) == 0) {
    PyErr_SetString(PyExc_KeyError, "empty property name");
    return NULL;
  }
  return SetProperty(self->object, name, value);
}

static PyObject* Entity_findByName(PyademEntity *self, PyObject *name) {
  const char *name_chars = PyString_AsString(name);

  if (name_chars == NULL)
    return NULL;

  Diadem::Entity *entity = self->object->FindByName(name_chars);

  if (entity == NULL)
    Py_RETURN_NONE;
  return reinterpret_cast<PyObject*>(WrapEntity(entity));
}

static PyObject* Entity_getPropertyByName(PyademEntity *self, PyObject *args) {
  if (self->object == NULL)
    return NULL;

  PyObject *entity_name, *prop_name;

  if (!PyArg_ParseTuple(args, "SS", &entity_name, &prop_name))
    return NULL;

  Diadem::Entity* const entity = GetEntityByName(self, entity_name);

  if (entity == NULL)
    return NULL;
  return GetProperty(entity, prop_name);
}

static PyObject* Entity_setPropertyByName(PyademEntity *self, PyObject *args) {
  if (self->object == NULL)
    return NULL;

  PyObject *entity_name, *prop_name, *value;

  if (!PyArg_ParseTuple(args, "SSO", &entity_name, &prop_name, &value))
    return NULL;

  Diadem::Entity* const entity = GetEntityByName(self, entity_name);

  if (entity == NULL)
    return NULL;
  return SetProperty(entity, prop_name, value);
}

static PyMethodDef Entity_methods[] = {
    { "GetProperty", (PyCFunction)Entity_getProperty, METH_O, NULL },
    { "SetProperty", (PyCFunction)Entity_setProperty, METH_VARARGS, NULL },
    { "FindByName",  (PyCFunction)Entity_findByName,  METH_O, NULL },
    { "SetPropertyByName", (PyCFunction)Entity_setPropertyByName,
      METH_VARARGS, NULL },
    { "GetPropertyByName", (PyCFunction)Entity_getPropertyByName,
      METH_VARARGS, NULL },
    { NULL },
    };

static PyMemberDef Entity_members[] = {
    { const_cast<char*>("button_callback"), T_OBJECT,
      offsetof(PyademEntity, button_callback), 0, NULL },
    { const_cast<char*>("context"), T_OBJECT,
      offsetof(PyademEntity, context), 0, NULL },
    { NULL },
    };

PyTypeObject EntityType = {
    PyObject_HEAD_INIT(NULL)
    0,                        /*ob_size*/
    "pyadem.Entity",          /*tp_name*/
    sizeof(PyademEntity),     /*tp_basicsize*/
    0,                        /*tp_itemsize*/
    (destructor)Entity_dealloc, /*tp_dealloc*/
    0,                        /*tp_print*/
    0,                        /*tp_getattr*/
    0,                        /*tp_setattr*/
    0,                        /*tp_compare*/
    0,                        /*tp_repr*/
    0,                        /*tp_as_number*/
    0,                        /*tp_as_sequence*/
    0,                        /*tp_as_mapping*/
    0,                        /*tp_hash*/
    0,                        /*tp_call*/
    0,                        /*tp_str*/
    0,                        /*tp_getattro*/
    0,                        /*tp_setattro*/
    0,                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,       /*tp_flags*/
    "Diadem Entity",          /*tp_doc*/
    0,                        /*tp_traverse*/
    0,                        /*tp_clear*/
    0,                        /*tp_richcompare*/
    0,                        /*tp_weaklistoffset*/
    0,                        /*tp_iter*/
    0,                        /*tp_iternext*/
    Entity_methods,           /*tp_methods*/
    Entity_members,           /*tp_members*/
    Entity_getsetters,        /*tp_getset*/
    0,                        /*tp_base*/
    0,                        /*tp_dict*/
    0,                        /*tp_descr_get*/
    0,                        /*tp_descr_set*/
    0,                        /*tp_dictoffset*/
    (initproc)Entity_init,    /*tp_init*/
    };

static int Window_init(PyademWindow *self, PyObject *args, PyObject *keywords) {
  if (Entity_init(&self->entity, args, keywords) < 0)
    return -1;
  self->window = new Diadem::Window(self->entity.object);
  if (!self->window->IsValid()) {
    PyErr_SetString(PyExc_RuntimeError, "invalid entity for window");
    return -1;
  }
  self->close_callback = NULL;
  self->window->SetCloseCallback(
      reinterpret_cast<Diadem::Window::CloseCallback>(&WindowCloseCallback),
      self);
  return 0;
}

static void Window_dealloc(PyademWindow *self) {
  if (self != NULL) {
    delete self->window;
    self->entity.object = NULL;  // deleted by ~Window
    Py_XDECREF(self->close_callback);
    Entity_dealloc(&self->entity);
  }
}

static PyObject* Window_showModeless(PyademWindow *self) {
  if ((self == NULL) || (self->window == NULL))
    return NULL;
  return PyBool_FromLong(self->window->ShowModeless());
}

static PyObject* Window_resizeToMinimum(PyademWindow *self) {
  if ((self == NULL) || (self->window == NULL))
    return NULL;
  self->window->GetRoot()->GetLayout()->ResizeToMinimum();
  Py_RETURN_NONE;
}

static PyObject* Window_close(PyademWindow *self) {
  if ((self == NULL) || (self->window == NULL))
    return NULL;
  return PyBool_FromLong(self->window->Close());
}

static PyObject* Window_showModal(PyademWindow *self) {
  if ((self == NULL) || (self->window == NULL))
    return NULL;

  bool result = self->window->ShowModal(NULL);

  if (PyErr_Occurred())
    return NULL;
  else
    return PyBool_FromLong(result);
}

static PyObject* Window_endModal(PyademWindow *self) {
  if ((self == NULL) || (self->window == NULL))
    return NULL;
  return PyBool_FromLong(self->window->EndModal());
}

static PyMethodDef Window_methods[] = {
    { "ShowModeless", (PyCFunction)Window_showModeless, METH_NOARGS, NULL },
    { "ResizeToMinimum", (PyCFunction)Window_resizeToMinimum,
      METH_NOARGS, NULL },
    { "Close",        (PyCFunction)Window_close,        METH_NOARGS, NULL },
    { "ShowModal",    (PyCFunction)Window_showModal,    METH_NOARGS, NULL },
    { "EndModal",     (PyCFunction)Window_endModal,     METH_NOARGS, NULL },
    { NULL },
    };

static PyMemberDef Window_members[] = {
    { const_cast<char*>("close_callback"), T_OBJECT,
      offsetof(PyademWindow, close_callback), 0, NULL },
    { NULL },
    };

PyTypeObject WindowType = {
    PyObject_HEAD_INIT(NULL)
    0,                        /*ob_size*/
    "pyadem.Window",          /*tp_name*/
    sizeof(PyademWindow),     /*tp_basicsize*/
    0,                        /*tp_itemsize*/
    (destructor)Window_dealloc,/*tp_dealloc*/
    0,                        /*tp_print*/
    0,                        /*tp_getattr*/
    0,                        /*tp_setattr*/
    0,                        /*tp_compare*/
    0,                        /*tp_repr*/
    0,                        /*tp_as_number*/
    0,                        /*tp_as_sequence*/
    0,                        /*tp_as_mapping*/
    0,                        /*tp_hash*/
    0,                        /*tp_call*/
    0,                        /*tp_str*/
    0,                        /*tp_getattro*/
    0,                        /*tp_setattro*/
    0,                        /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,       /*tp_flags*/
    "Diadem Window",          /*tp_doc*/
    0,                        /*tp_traverse*/
    0,                        /*tp_clear*/
    0,                        /*tp_richcompare*/
    0,                        /*tp_weaklistoffset*/
    0,                        /*tp_iter*/
    0,                        /*tp_iternext*/
    Window_methods,           /*tp_methods*/
    Window_members,           /*tp_members*/
    0,                        /*tp_getset*/
    &EntityType,              /*tp_base*/
    0,                        /*tp_dict*/
    0,                        /*tp_descr_get*/
    0,                        /*tp_descr_set*/
    0,                        /*tp_dictoffset*/
    (initproc)Window_init,    /*tp_init*/
    };

static PyObject* ChooseFolder(PyObject *self, PyObject *args) {
  PyObject *start_string;
  Diadem::String start_folder;

  if (PyArg_ParseTuple(args, "S", &start_string))
    start_folder = PyString_AsString(start_string);

  const Diadem::String result = DIADEM_PLATFORM::ChooseFolder(start_folder);

  if (result.IsEmpty())
    Py_RETURN_NONE;
  return PyString_FromString(result.Get());
}

static PyObject* ChooseNewPath(
    PyObject *self, PyObject *args, PyObject *keywords) {
  static const char *keys[] = { "prompt", "path", "name", NULL };
  PyObject *prompt = NULL, *path = NULL, *name = NULL;

  if (!PyArg_ParseTupleAndKeywords(
      args, keywords, "|SSS", const_cast<char**>(keys),
      &prompt, &path, &name))
    return NULL;

  const Diadem::String result = DIADEM_PLATFORM::ChooseNewPath(
      Diadem::String((prompt == NULL) ? "" : PyString_AsString(prompt)),
      Diadem::String((path == NULL) ? "" : PyString_AsString(path)),
      Diadem::String((name == NULL) ? "" : PyString_AsString(name)));

  if (result.IsEmpty())
    Py_RETURN_NONE;
  return PyString_FromString(result.Get());
}

static bool IsValidButtonData(PyObject *data) {
  return (data != NULL) && (PyString_Check(data) || PyObject_IsTrue(data));
}

static void MessageCallback(Diadem::ButtonType button, void *data) {
  GILState gil;
  PyObject *callback = reinterpret_cast<PyObject*>(data);
  const char *button_name = ButtonNameFromButtonType(button);
  PyObject *args = PyTuple_Pack(1, PyString_FromString(button_name));

  PyObject_CallObject(callback, args);
  if (PyErr_Occurred()) {
    // Can't propagate an exception from this callback, so just log it.
    PyErr_Print();
    PyErr_Clear();
  }
  Py_DECREF(callback);
  Py_DECREF(args);
}

static PyObject* ShowMessage(
    PyObject *self, PyObject *args, PyObject *keywords) {
  static const char *keys[] = {
      "message", "accept", "cancel", "other", "suppress", "callback", NULL };
  PyObject *message = NULL, *accept = NULL, *cancel = NULL, *other = NULL,
      *suppress = NULL, *callback = NULL;

  if (!PyArg_ParseTupleAndKeywords(
      args, keywords, "S|SOOOO", const_cast<char**>(keys),
      &message, &accept, &cancel, &other, &suppress, &callback))
    return NULL;

  Diadem::MessageData data(PyString_AsString(message));

  if (accept != NULL)
    data.accept_text_ = PyString_AsString(accept);
  if (IsValidButtonData(cancel)) {
    data.show_cancel_ = true;
    if (PyString_Check(cancel))
      data.cancel_text_ = PyString_AsString(cancel);
  }
  if (IsValidButtonData(other)) {
    data.show_other_ = true;
    if (PyString_Check(other))
      data.other_text_ = PyString_AsString(other);
  }
  if (IsValidButtonData(suppress)) {
    data.show_suppress_ = true;
    if (PyString_Check(suppress))
      data.suppress_text_ = PyString_AsString(suppress);
  }
  if (callback != NULL) {
    if (!PyCallable_Check(callback)) {
      PyErr_SetString(PyExc_TypeError, "callback is not callable");
      return NULL;
    }
    Py_INCREF(callback);
    data.callback_ = MessageCallback;
    data.callback_data_ = callback;
  }

  const Diadem::ButtonType button = DIADEM_PLATFORM::ShowMessage(&data);
  const char *button_name = ButtonNameFromButtonType(button);

  if (data.show_suppress_)
    return Py_BuildValue("si", button_name, data.suppressed_);
  else
    return PyString_FromString(button_name);
}

static PyMethodDef Pyadem_Methods[] = {
    { "ChooseFolder", ChooseFolder, METH_VARARGS,
      "Asks the user to choose a folder." },
    { "ChooseNewPath", (PyCFunction)ChooseNewPath, METH_VARARGS | METH_KEYWORDS,
      "Displays a Save As prompt." },
    { "ShowMessage", (PyCFunction)ShowMessage, METH_VARARGS | METH_KEYWORDS,
      "Displays a message or prompt." },
    { NULL },
    };

// Since the name of the module is "pyadem", Python will look for a function
// named "initpyadem".
PyMODINIT_FUNC
initpyadem() {
  EntityType.tp_new = PyType_GenericNew;
  WindowType.tp_new = PyType_GenericNew;
  if (PyType_Ready(&EntityType) < 0)
    return;
  if (PyType_Ready(&WindowType) < 0)
    return;

  PyObject *module = Py_InitModule3("pyadem", Pyadem_Methods, NULL);

  Py_INCREF(&EntityType);
  PyModule_AddObject(
      module, "Entity", reinterpret_cast<PyObject*>(&EntityType));
  PyModule_AddObject(
      module, "Window", reinterpret_cast<PyObject*>(&WindowType));

  PyModule_AddStringConstant(
      module, "PROP_TEXT", Diadem::kPropText);
  PyModule_AddStringConstant(
      module, "PROP_ENABLED", Diadem::kPropEnabled);
  PyModule_AddStringConstant(
      module, "PROP_VISIBLE", Diadem::kPropVisible);
  PyModule_AddStringConstant(
      module, "PROP_IN_LAYOUT", Diadem::kPropInLayout);
  PyModule_AddStringConstant(
      module, "PROP_URL", Diadem::kPropURL);
  PyModule_AddStringConstant(
      module, "PROP_VALUE", Diadem::kPropValue);
  PyModule_AddStringConstant(
      module, "PROP_ROW_COUNT", Diadem::kPropRowCount);
  PyModule_AddStringConstant(
      module, "PROP_DATA", Diadem::kPropData);

  PyModule_AddStringConstant(module, "BUTTON_ACCEPT", kButtonNameAccept);
  PyModule_AddStringConstant(module, "BUTTON_CANCEL", kButtonNameCancel);
  PyModule_AddStringConstant(module, "BUTTON_OTHER", kButtonNameOther);
}
