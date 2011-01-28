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
#include "Diadem/NativeCocoa.h"
#include "Diadem/Value.h"

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

  Diadem::Cocoa::SetUpFactory(&factory);

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

static PyObject* Entity_getProperty(PyademEntity *self, PyObject *name) {
  if (self->object == NULL)
    return NULL;

  PyObject *result =
      self->object->GetProperty(PyString_AsString(name)).Coerce<PyObject*>();

  // Sometimes PyString_AsString sets the error indicator even when it succeeds.
  PyErr_Clear();
  return result;
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
  if (!self->object->SetProperty(PyString_AsString(name), value)) {
    PyErr_SetString(PyExc_KeyError, "property not found");
    return NULL;
  }
  PyErr_Clear();
  return Py_None;
}

static PyObject* Entity_findByName(PyademEntity *self, PyObject *name) {
  const char *name_chars = PyString_AsString(name);

  if (name_chars == NULL)
    return NULL;

  Diadem::Entity *entity = self->object->FindByName(name_chars);

  if (entity == NULL)
    return Py_None;
  return (PyObject*)WrapEntity(entity);
}

static PyMethodDef Entity_methods[] = {
    { "GetProperty", (PyCFunction)Entity_getProperty, METH_O, NULL },
    { "SetProperty", (PyCFunction)Entity_setProperty, METH_VARARGS, NULL },
    { "FindByName",  (PyCFunction)Entity_findByName,  METH_O, NULL },
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
  }
  Py_XDECREF(self->close_callback);
  Entity_dealloc(&self->entity);
}

static PyObject* Window_showModeless(PyademWindow *self) {
  if ((self == NULL) || (self->window == NULL))
    return NULL;
  return PyBool_FromLong(self->window->ShowModeless());
}

static PyObject* Window_close(PyademWindow *self) {
  if ((self == NULL) || (self->window == NULL))
    return NULL;
  return PyBool_FromLong(self->window->Close());
}

static PyObject* Window_showModal(PyademWindow *self) {
  if ((self == NULL) || (self->window == NULL))
    return NULL;

  Diadem::Bool result = self->window->ShowModal(NULL);

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

  const Diadem::String result = Diadem::Cocoa::ChooseFolder(start_folder);

  if (strlen(result.Get()) == 0)
    return Py_None;
  return PyString_FromString(result.Get());
}

static bool IsValidButtonData(PyObject *data) {
  return (data != NULL) && (PyString_Check(data) || PyObject_IsTrue(data));
}

static PyObject* ShowMessage(
    PyObject *self, PyObject *args, PyObject *keywords) {
  static const char *keys[] = {
      "message", "accept", "cancel", "other", "suppress", NULL };
  PyObject *message = NULL, *accept = NULL, *cancel = NULL, *other = NULL,
      *suppress = NULL;

  if (!PyArg_ParseTupleAndKeywords(
      args, keywords, "S|SOOO", const_cast<char**>(keys),
      &message, &accept, &cancel, &other, &suppress))
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

  const Diadem::ButtonType button = Diadem::Cocoa::ShowMessage(&data);
  const char *button_name;

  switch (button) {
    case Diadem::kAcceptButton:
      button_name = "accept";
      break;
    case Diadem::kOtherButton:
      button_name = "other";
      break;
    case Diadem::kCancelButton:
    default:
      button_name = "cancel";
      break;
  }

  if (data.show_suppress_)
    return Py_BuildValue("si", button_name, data.suppressed_);
  else
    return PyString_FromString(button_name);
}

static PyMethodDef Pyadem_Methods[] = {
    { "ChooseFolder", ChooseFolder, METH_VARARGS,
      "Ask the user to choose a folder." },
    { "ShowMessage", (PyCFunction)ShowMessage, METH_VARARGS | METH_KEYWORDS,
      "Display a message or prompt." },
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
}
