/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the V4VM module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
// Copyright 2012 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

/** \mainpage V8 API Reference Guide
 *
 * V8 is Google's open source JavaScript engine.
 *
 * This set of documents provides reference material generated from the
 * V8 header file, include/v8.h.
 *
 * For other documentation see http://code.google.com/apis/v8/
 */

#ifndef V8_H_
#define V8_H_

#include "qv4global.h"
#include "qv4string.h"
#include <QStack>
#include <QSharedData>

namespace QQmlJS {
namespace VM {
struct Value;
struct String;
struct ExecutionEngine;
struct Object;
class MemoryManager;
}
}

#include <stdint.h>

#define V8EXPORT Q_V4_EXPORT

/**
 * The v8 JavaScript engine.
 */
namespace v8 {

class Context;
class String;
class StringObject;
class Value;
class Utils;
class Number;
class NumberObject;
class Object;
class Array;
class Int32;
class Uint32;
class External;
class Primitive;
class Boolean;
class BooleanObject;
class Integer;
class Function;
class Date;
class ImplementationUtilities;
class Signature;
class AccessorSignature;
template <class T> struct Handle;
template <class T> class Local;
template <class T> class Persistent;
class FunctionTemplate;
class ObjectTemplate;
class Data;
class AccessorInfo;
class StackTrace;
class StackFrame;
class Isolate;
class TryCatch;

V8EXPORT void *gcProtect(void *handle);
V8EXPORT void gcProtect(void *memoryManager, void *handle);
V8EXPORT void gcUnprotect(void *memoryManager, void *handle);

// --- Weak Handles ---

/**
 * A weak reference callback function.
 *
 * This callback should either explicitly invoke Dispose on |object| if
 * V8 wrapper is not needed anymore, or 'revive' it by invocation of MakeWeak.
 *
 * \param object the weak global object to be reclaimed by the garbage collector
 * \param parameter the value passed in when making the weak global object
 */
typedef void (*WeakReferenceCallback)(Persistent<Value> object,
                                      void* parameter);


// --- Handles ---

#define TYPE_CHECK(T, S)                                       \
  while (false) {                                              \
    *(static_cast<T* volatile*>(0)) = static_cast<S*>(0);      \
  }

/**
 * An object reference managed by the v8 garbage collector.
 *
 * All objects returned from v8 have to be tracked by the garbage
 * collector so that it knows that the objects are still alive.  Also,
 * because the garbage collector may move objects, it is unsafe to
 * point directly to an object.  Instead, all objects are stored in
 * handles which are known by the garbage collector and updated
 * whenever an object moves.  Handles should always be passed by value
 * (except in cases like out-parameters) and they should never be
 * allocated on the heap.
 *
 * There are two types of handles: local and persistent handles.
 * Local handles are light-weight and transient and typically used in
 * local operations.  They are managed by HandleScopes.  Persistent
 * handles can be used when storing objects across several independent
 * operations and have to be explicitly deallocated when they're no
 * longer used.
 *
 * It is safe to extract the object stored in the handle by
 * dereferencing the handle (for instance, to extract the Object* from
 * a Handle<Object>); the value will still be governed by a handle
 * behind the scenes and the same rules apply to these values as to
 * their handles.
 */

template <typename T>
struct Handle;

template <typename T>
struct HandleOperations
{
    static void init(Handle<T> *handle)
    {
#if QT_POINTER_SIZE == 8
        handle->val = quint64(Handle<T>::_Null_Type) << Handle<T>::Tag_Shift;
#else
        handle->tag = Handle<T>::_Null_Type;
        handle->int_32 = 0;
#endif
    }

    static void ref(Handle<T> *)
    {
    }

    static void deref(Handle<T> *)
    {
    }

    static void *protect(Handle<T> *handle)
    {
        return gcProtect(handle);
    }

    static void protect(void *memoryManager, Handle<T> *handle)
    {
        gcProtect(memoryManager, handle);
    }

    static void unProtect(void *memoryManager, Handle<T> *handle)
    {
        gcUnprotect(memoryManager, handle);
    }

    static bool isEmpty(const Handle<T> *handle)
    {
        return handle->tag == Handle<T>::_Null_Type;
    }

    static T *get(const Handle<T> *handle)
    {
        return const_cast<T*>(reinterpret_cast<const T*>(handle));
    }
};

#define DEFINE_REFCOUNTED_HANDLE_OPERATIONS(Type) \
    template <> \
    struct HandleOperations<Type> \
    { \
        static void init(Handle<Type> *handle) \
        { \
            handle->object = 0; \
        } \
    \
        static void ref(Handle<Type> *handle) \
        { \
            if (handle->object) \
                handle->object->ref.ref(); \
        } \
    \
        static void deref(Handle<Type> *handle) \
        { \
            if (handle->object && !handle->object->ref.deref()) { \
                delete handle->object; \
                handle->object = 0; \
            } \
        } \
        static void *protect(Handle<Type> *) { return 0; } \
        static void protect(void *, Handle<Type> *) {} \
        static void unProtect(void *, Handle<Type> *) {} \
        static bool isEmpty(const Handle<Type> *handle) \
        { \
            return handle->object == 0; \
        } \
        static Type *get(const Handle<Type> *handle) \
        { \
        return handle->object; \
        } \
     \
    };

template <typename T>
struct Handle {
    Handle()
    {
        HandleOperations<T>::init(this);
    }
    template <typename Other>
    Handle(const Handle<Other> &that)
        : val(that.val)
    {
        HandleOperations<T>::ref(this);
    }

    explicit Handle(T *obj)
    {
        object = obj;
        HandleOperations<T>::ref(this);
    }

    Handle(const Handle<T> &other)
        : val(other.val)
    {
        HandleOperations<T>::ref(this);
    }
    Handle<T> &operator=(const Handle<T> &other)
    {
        if (this == &other)
            return *this;
        HandleOperations<T>::deref(this);
        this->val = other.val;
        HandleOperations<T>::ref(this);
        return *this;
    }
    ~Handle()
    {
        HandleOperations<T>::deref(this);
    }

    bool IsEmpty() const { return HandleOperations<T>::isEmpty(this); }

    T *operator->() const { return HandleOperations<T>::get(this); }

    T *get() const { return HandleOperations<T>::get(this); }

    template <typename Source>
    static Handle<T> Cast(Handle<Source> that)
    {
        return that.template As<T>();
    }

    template <typename Target>
    Handle<Target> As()
    {
        return Handle<Target>(*this);
    }

    void Clear()
    {
        val = 0;
    }

    template <class S> inline bool operator==(Handle<S> that) const {
        return val == that.val;
    }
    template <class S> inline bool operator!=(Handle<S> that) const {
        return val != that.val;
    }

    enum Masks {
        NotDouble_Mask = 0xfffc0000,
        Type_Mask = 0xffff8000,
        Immediate_Mask = NotDouble_Mask | 0x00008000,
        Tag_Shift = 32
    };

    enum ValueType {
        Undefined_Type = Immediate_Mask | 0x00000,
        Null_Type = Immediate_Mask | 0x10000,
        Boolean_Type = Immediate_Mask | 0x20000,
        Integer_Type = Immediate_Mask | 0x30000,
        Object_Type = NotDouble_Mask | 0x00000,
        String_Type = NotDouble_Mask | 0x10000
    };

    enum ImmediateFlags {
        ConvertibleToInt = Immediate_Mask | 0x1
    };

    enum ValueTypeInternal {
        _Undefined_Type = Undefined_Type,
        _Null_Type = Null_Type | ConvertibleToInt,
        _Boolean_Type = Boolean_Type | ConvertibleToInt,
        _Integer_Type = Integer_Type | ConvertibleToInt,
        _Object_Type = Object_Type,
        _String_Type = String_Type

    };

    union {
        T *object;
        quint64 val;
        double dbl;
        struct {
#if Q_BYTE_ORDER != Q_LITTLE_ENDIAN
            uint tag;
#endif
            union {
                uint uint_32;
                int int_32;
#if QT_POINTER_SIZE == 4
                T *o;
#endif
            };
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
            uint tag;
#endif
        };
    };
};


/**
 * A light-weight stack-allocated object handle.  All operations
 * that return objects from within v8 return them in local handles.  They
 * are created within HandleScopes, and all local handles allocated within a
 * handle scope are destroyed when the handle scope is destroyed.  Hence it
 * is not necessary to explicitly deallocate local handles.
 */
template <class T> class Local : public Handle<T> {
 public:
  Local() {}
  template <class S> Local(Local<S> that)
      : Handle<T>(Handle<T>::Cast(that)) {
    /**
     * This check fails when trying to convert between incompatible
     * handles. For example, converting from a Handle<String> to a
     * Handle<Number>.
     */
    TYPE_CHECK(T, S);
  }
  template <class S> Local(S* that) : Handle<T>(that) { }
  template <class S> static Local<T> Cast(Local<S> that) {
#ifdef V8_ENABLE_CHECKS
    // If we're going to perform the type check then we have to check
    // that the handle isn't empty before doing the checked cast.
    if (that.IsEmpty()) return Local<T>();
#endif
    return Local<T>::New(Handle<T>::Cast(that));
  }

  template <class S> Local<S> As() {
    return Local<S>::Cast(*this);
  }

  /** Create a local handle for the content of another handle.
   *  The referee is kept alive by the local handle even when
   *  the original handle is destroyed/disposed.
   */
  static Local<T> New(Handle<T> that)
  {
      Local<T> result;
      result.Handle<T>::operator =(that);
      return result;
  }
};


/**
 * An object reference that is independent of any handle scope.  Where
 * a Local handle only lives as long as the HandleScope in which it was
 * allocated, a Persistent handle remains valid until it is explicitly
 * disposed.
 *
 * A persistent handle contains a reference to a storage cell within
 * the v8 engine which holds an object value and which is updated by
 * the garbage collector whenever the object is moved.  A new storage
 * cell can be created using Persistent::New and existing handles can
 * be disposed using Persistent::Dispose.  Since persistent handles
 * are passed by value you may have many persistent handle objects
 * that point to the same storage cell.  For instance, if you pass a
 * persistent handle as an argument to a function you will not get two
 * different storage cells but rather two references to the same
 * storage cell.
 */
template <class T> class Persistent : public Handle<T> {
 public:
  /**
   * Creates an empty persistent handle that doesn't point to any
   * storage cell.
   */
  Persistent() {}
  ~Persistent() {
      HandleOperations<T>::unProtect(m_memoryManager, this);
  }

  Persistent(const Persistent &other)
      : Handle<T>(other)
      , m_memoryManager(other.m_memoryManager)
  {
      HandleOperations<T>::protect(m_memoryManager, this);
  }

  Persistent &operator =(const Persistent &other)
  {
      if (&other == this)
          return *this;
      HandleOperations<T>::unProtect(m_memoryManager, this);
      Handle<T>::operator =(other);
      m_memoryManager = other.m_memoryManager;
      HandleOperations<T>::protect(m_memoryManager, this);
      return *this;
  }

  /**
   * Creates a persistent handle for the same storage cell as the
   * specified handle.  This constructor allows you to pass persistent
   * handles as arguments by value and to assign between persistent
   * handles.  However, attempting to assign between incompatible
   * persistent handles, for instance from a Persistent<String> to a
   * Persistent<Number> will cause a compile-time error.  Assigning
   * between compatible persistent handles, for instance assigning a
   * Persistent<String> to a variable declared as Persistent<Value>,
   * is allowed as String is a subclass of Value.
   */
  template <class S> Persistent(Persistent<S> that)
      : Handle<T>(Handle<T>::Cast(that)) {
      m_memoryManager = that.m_memoryManager;
      HandleOperations<T>::protect(m_memoryManager, this);
  }

  template <class S> Persistent(S* that) : Handle<T>(that)
  {
      m_memoryManager = HandleOperations<T>::protect(this);
  }

  /**
   * "Casts" a plain handle which is known to be a persistent handle
   * to a persistent handle.
   */
  template <class S> explicit Persistent(Handle<S> that)
      : Handle<T>(*that)
  {
      m_memoryManager = HandleOperations<T>::protect(this);
  }

  template <class S> static Persistent<T> Cast(Persistent<S> that) {
    return Persistent<T>(T::Cast(*that));
  }

  template <class S> Persistent<S> As() {
    return Persistent<S>::Cast(*this);
  }

  /**
   * Creates a new persistent handle for an existing local or
   * persistent handle.
   */
  static Persistent<T> New(Handle<T> that)
  {
      Persistent<T> result;
      result.Handle<T>::operator =(that);
      result.m_memoryManager = HandleOperations<T>::protect(&result);
      return result;
  }

  /**
   * Releases the storage cell referenced by this persistent handle.
   * Does not remove the reference to the cell from any handles.
   * This handle's reference, and any other references to the storage
   * cell remain and IsEmpty will still return false.
   */
  void Dispose() {
       HandleOperations<T>::unProtect(m_memoryManager, this);
       m_memoryManager = 0;
       HandleOperations<T>::deref(this);
       HandleOperations<T>::init(this);
  }

  void Dispose(Isolate*) {
      Dispose();
  }

  /**
   * Make the reference to this object weak.  When only weak handles
   * refer to the object, the garbage collector will perform a
   * callback to the given V8::WeakReferenceCallback function, passing
   * it the object reference and the given parameters.
   */
  void MakeWeak(void* parameters, WeakReferenceCallback callback);
public:
  void *m_memoryManager;
};


 /**
 * A stack-allocated class that governs a number of local handles.
 * After a handle scope has been created, all local handles will be
 * allocated within that handle scope until either the handle scope is
 * deleted or another handle scope is created.  If there is already a
 * handle scope and a new one is created, all allocations will take
 * place in the new handle scope until it is deleted.  After that,
 * new handles will again be allocated in the original handle scope.
 *
 * After the handle scope of a local handle has been deleted the
 * garbage collector will no longer track the object stored in the
 * handle and may deallocate it.  The behavior of accessing a handle
 * for which the handle scope has been deleted is undefined.
 */
class V8EXPORT HandleScope {
 public:
  HandleScope() {}

  ~HandleScope() {}

  /**
   * Closes the handle scope and returns the value as a handle in the
   * previous scope, which is the new current scope after the call.
   */
  template <class T> Local<T> Close(Handle<T> value) { return Local<T>::New(value); }
};


// --- Special objects ---


/**
 * The superclass of values and API object templates.
 */
class V8EXPORT Data  : public QSharedData {
};

DEFINE_REFCOUNTED_HANDLE_OPERATIONS(Data)

/**
 * The origin, within a file, of a script.
 */
class V8EXPORT ScriptOrigin {
public:
  ScriptOrigin() : m_lineNumber(0), m_columnNumber(0) {}

  ScriptOrigin(
      Handle<Value> resource_name,
      Handle<Integer> resource_line_offset = Handle<Integer>(),
      Handle<Integer> resource_column_offset = Handle<Integer>());
  Handle<Value> ResourceName() const;
  Handle<Integer> ResourceLineOffset() const;
  Handle<Integer> ResourceColumnOffset() const;
private:
  QString m_fileName;
  int m_lineNumber, m_columnNumber;
  friend class Script;
};

class ScriptData;

/**
 * A compiled JavaScript script.
 */
class V8EXPORT Script : public QSharedData {
 public:
  enum CompileFlags {
      Default    = 0x00,
      QmlMode    = 0x01,
      NativeMode = 0x02
  };

  /**
   * Compiles the specified script (context-independent).
   *
   * \param source Script source code.
   * \param origin Script origin, owned by caller, no references are kept
   *   when New() returns
   * \param pre_data Pre-parsing data, as obtained by ScriptData::PreCompile()
   *   using pre_data speeds compilation if it's done multiple times.
   *   Owned by caller, no references are kept when New() returns.
   * \param script_data Arbitrary data associated with script. Using
   *   this has same effect as calling SetData(), but allows data to be
   *   available to compile event handlers.
   * \return Compiled script object (context independent; when run it
   *   will use the currently entered context).
   */
  static Local<Script> New(Handle<String> source,
                           ScriptOrigin* origin = NULL,
                           ScriptData* pre_data = NULL,
                           Handle<String> script_data = Handle<String>(),
                           CompileFlags = Default);

  /**
   * Compiles the specified script using the specified file name
   * object (typically a string) as the script's origin.
   *
   * \param source Script source code.
   * \param file_name file name object (typically a string) to be used
   *   as the script's origin.
   * \return Compiled script object (context independent; when run it
   *   will use the currently entered context).
   */
  static Local<Script> New(Handle<String> source,
                           Handle<Value> file_name,
                           CompileFlags = Default);

  /**
   * Compiles the specified script (bound to current context).
   *
   * \param source Script source code.
   * \param origin Script origin, owned by caller, no references are kept
   *   when Compile() returns
   * \param pre_data Pre-parsing data, as obtained by ScriptData::PreCompile()
   *   using pre_data speeds compilation if it's done multiple times.
   *   Owned by caller, no references are kept when Compile() returns.
   * \param script_data Arbitrary data associated with script. Using
   *   this has same effect as calling SetData(), but makes data available
   *   earlier (i.e. to compile event handlers).
   * \return Compiled script object, bound to the context that was active
   *   when this function was called.  When run it will always use this
   *   context.
   */
  static Local<Script> Compile(Handle<String> source,
                               ScriptOrigin* origin = NULL,
                               ScriptData* pre_data = NULL,
                               Handle<String> script_data = Handle<String>(),
                               CompileFlags = Default);

  /**
   * Compiles the specified script using the specified file name
   * object (typically a string) as the script's origin.
   *
   * \param source Script source code.
   * \param file_name File name to use as script's origin
   * \param script_data Arbitrary data associated with script. Using
   *   this has same effect as calling SetData(), but makes data available
   *   earlier (i.e. to compile event handlers).
   * \return Compiled script object, bound to the context that was active
   *   when this function was called.  When run it will always use this
   *   context.
   */
  static Local<Script> Compile(Handle<String> source,
                               Handle<Value> file_name,
                               Handle<String> script_data = Handle<String>(),
                               CompileFlags = Default);

  /**
   * Runs the script returning the resulting value.  If the script is
   * context independent (created using ::New) it will be run in the
   * currently entered context.  If it is context specific (created
   * using ::Compile) it will be run in the context in which it was
   * compiled.
   */
  Local<Value> Run();
  Local<Value> Run(Handle<Object> qml);

  /**
   * Returns the script id value.
   */
  Local<Value> Id();

  /**
   * Associate an additional data object with the script. This is mainly used
   * with the debugger as this data object is only available through the
   * debugger API.
   */
  void SetData(Handle<String> data);

private:
  QString m_script;
  ScriptOrigin m_origin;
  CompileFlags m_flags;
  Handle<Context> m_context;
};

DEFINE_REFCOUNTED_HANDLE_OPERATIONS(Script)

/**
 * An error message.
 */
class V8EXPORT Message : public QSharedData {
 public:
    Message(const QString &message, const QString &resourceName, int lineNumber)
        : m_message(message), m_resourceName(resourceName), m_lineNumber(lineNumber) {}

  Local<String> Get() const;
  /**
   * Returns the resource name for the script from where the function causing
   * the error originates.
   */
  Handle<Value> GetScriptResourceName() const;

  /**
   * Returns the number, 1-based, of the line where the error occurred.
   */
  int GetLineNumber() const;

private:
  QString m_message;
  QString m_resourceName;
  int m_lineNumber;
};

DEFINE_REFCOUNTED_HANDLE_OPERATIONS(Message)

/**
 * Representation of a JavaScript stack trace. The information collected is a
 * snapshot of the execution stack and the information remains valid after
 * execution continues.
 */
class V8EXPORT StackTrace : public QSharedData
{
 public:
  /**
   * Flags that determine what information is placed captured for each
   * StackFrame when grabbing the current stack trace.
   */
  enum StackTraceOptions {
    kLineNumber = 1,
    kColumnOffset = 1 << 1 | kLineNumber,
    kScriptName = 1 << 2,
    kFunctionName = 1 << 3,
    kIsEval = 1 << 4,
    kIsConstructor = 1 << 5,
    kScriptNameOrSourceURL = 1 << 6,
    kOverview = kLineNumber | kColumnOffset | kScriptName | kFunctionName,
    kDetailed = kOverview | kIsEval | kIsConstructor | kScriptNameOrSourceURL
  };

  /**
   * Returns a StackFrame at a particular index.
   */
  Local<StackFrame> GetFrame(uint32_t index) const;

  /**
   * Returns the number of StackFrames.
   */
  int GetFrameCount() const;

  /**
   * Returns StackTrace as a v8::Array that contains StackFrame objects.
   */
  Local<Array> AsArray();

  /**
   * Grab a snapshot of the current JavaScript execution stack.
   *
   * \param frame_limit The maximum number of stack frames we want to capture.
   * \param options Enumerates the set of things we will capture for each
   *   StackFrame.
   */
  static Local<StackTrace> CurrentStackTrace(
      int frame_limit,
      StackTraceOptions options = kOverview);

  private:
  QVector<Local<StackFrame> > frames;
};

DEFINE_REFCOUNTED_HANDLE_OPERATIONS(StackTrace)


/**
 * A single JavaScript stack frame.
 */
class V8EXPORT StackFrame : public QSharedData {
 public:
  /**
   * Returns the number, 1-based, of the line for the associate function call.
   * This method will return Message::kNoLineNumberInfo if it is unable to
   * retrieve the line number, or if kLineNumber was not passed as an option
   * when capturing the StackTrace.
   */
  int GetLineNumber() const;

  /**
   * Returns the 1-based column offset on the line for the associated function
   * call.
   * This method will return Message::kNoColumnInfo if it is unable to retrieve
   * the column number, or if kColumnOffset was not passed as an option when
   * capturing the StackTrace.
   */
  int GetColumn() const;

  /**
   * Returns the name of the resource that contains the script for the
   * function for this StackFrame.
   */
  Local<String> GetScriptName() const;

  /**
   * Returns the name of the resource that contains the script for the
   * function for this StackFrame or sourceURL value if the script name
   * is undefined and its source ends with //@ sourceURL=... string.
   */
  Local<String> GetScriptNameOrSourceURL() const;

  /**
   * Returns the name of the function associated with this stack frame.
   */
  Local<String> GetFunctionName() const;

private:
  friend class StackTrace;
  StackFrame(Handle<String> script, Handle<String> function, int line, int column);
  int m_lineNumber;
  int m_columnNumber;
  Persistent<String> m_scriptName;
  Persistent<String> m_functionName;
};

DEFINE_REFCOUNTED_HANDLE_OPERATIONS(StackFrame)

// --- Value ---


/**
 * The superclass of all JavaScript values and objects.
 */
class V8EXPORT Value {
 public:
  /**
   * Returns true if this value is the undefined value.  See ECMA-262
   * 4.3.10.
   */
  bool IsUndefined() const;

  /**
   * Returns true if this value is the null value.  See ECMA-262
   * 4.3.11.
   */
  bool IsNull() const;

   /**
   * Returns true if this value is true.
   */
  bool IsTrue() const;

  /**
   * Returns true if this value is false.
   */
  bool IsFalse() const;

  /**
   * Returns true if this value is an instance of the String type.
   * See ECMA-262 8.4.
   */
  bool IsString() const;

  /**
   * Returns true if this value is a function.
   */
  bool IsFunction() const;

  /**
   * Returns true if this value is an array.
   */
  bool IsArray() const;

  /**
   * Returns true if this value is an object.
   */
  bool IsObject() const;

  /**
   * Returns true if this value is boolean.
   */
  bool IsBoolean() const;

  /**
   * Returns true if this value is a number.
   */
  bool IsNumber() const;

  /**
   * Returns true if this value is external.
   */
  bool IsExternal() const;

  /**
   * Returns true if this value is a 32-bit signed integer.
   */
  bool IsInt32() const;

  /**
   * Returns true if this value is a 32-bit unsigned integer.
   */
  bool IsUint32() const;

  /**
   * Returns true if this value is a Date.
   */
  bool IsDate() const;

  /**
   * Returns true if this value is a Boolean object.
   */
  bool IsBooleanObject() const;

  /**
   * Returns true if this value is a Number object.
   */
  bool IsNumberObject() const;

  /**
   * Returns true if this value is a String object.
   */
  bool IsStringObject() const;

  /**
   * Returns true if this value is a RegExp.
   */
  bool IsRegExp() const;

  /**
   * Returns true if this value is an Error.
   */
  bool IsError() const;

  Local<Boolean> ToBoolean() const;
  Local<Number> ToNumber() const;
  Local<String> ToString() const;
  Local<Object> ToObject() const;
  Local<Integer> ToInteger() const;
  Local<Uint32> ToUint32() const;
  Local<Int32> ToInt32() const;

  /**
   * Attempts to convert a string to an array index.
   * Returns an empty handle if the conversion fails.
   */
  Local<Uint32> ToArrayIndex() const;

  bool BooleanValue() const;
  double NumberValue() const;
  int64_t IntegerValue() const;
  uint32_t Uint32Value() const;
  int32_t Int32Value() const;

  /** JS == */
  bool Equals(Handle<Value> that) const;
  bool StrictEquals(Handle<Value> that) const;

  static Handle<Value> NewFromInternalValue(quint64 val)
  {
      Handle<Value> res;
      res.val = val;
      return res;
  }

  QQmlJS::VM::Value vmValue() const;
  static Handle<Value> fromVmValue(const QQmlJS::VM::Value &vmValue);

};


/**
 * The superclass of primitive values.  See ECMA-262 4.3.2.
 */
class V8EXPORT Primitive : public Value { };


/**
 * A primitive boolean value (ECMA-262, 4.3.14).  Either the true
 * or false value.
 */
class V8EXPORT Boolean : public Primitive {
 public:
  bool Value() const;
  static Handle<Boolean> New(bool value);
};


/**
 * A JavaScript string value (ECMA-262, 4.3.17).
 */
class V8EXPORT String : public Primitive {
 public:
  /**
   * Returns the number of characters in this string.
   */
  int Length() const;


  /**
   * Returns the hash of this string.
   */
  uint32_t Hash() const;

  struct CompleteHashData {
    CompleteHashData() : length(0), hash(0), symbol_id(0) {}
    int length;
    uint32_t hash;
    uint32_t symbol_id;
  };

  /**
   * Returns the "complete" hash of the string.  This is
   * all the information about the string needed to implement
   * a very efficient hash keyed on the string.
   *
   * The members of CompleteHashData are:
   *    length: The length of the string.  Equivalent to Length()
   *    hash: The hash of the string.  Equivalent to Hash()
   *    symbol_id: If the string is a sequential symbol, the symbol
   *        id, otherwise 0.  If the symbol ids of two strings are
   *        the same (and non-zero) the two strings are identical.
   *        If the symbol ids are different the strings may still be
   *        identical, but an Equals() check must be performed.
   */
  CompleteHashData CompleteHash() const;

  /**
   * Compute a hash value for the passed UTF16 string
   * data.
   */
  static uint32_t ComputeHash(uint16_t *string, int length);
  static uint32_t ComputeHash(char *string, int length);

  /**
   * Returns true if this string is equal to the external
   * string data provided.
   */
  bool Equals(uint16_t *string, int length);
  bool Equals(char *string, int length);
  bool Equals(Handle<Value> that) const {
    return v8::Value::Equals(that);
  }

  /**
   * Write the contents of the string to an external buffer.
   * If no arguments are given, expects the buffer to be large
   * enough to hold the entire string and NULL terminator. Copies
   * the contents of the string and the NULL terminator into the
   * buffer.
   *
   * WriteUtf8 will not write partial UTF-8 sequences, preferring to stop
   * before the end of the buffer.
   *
   * Copies up to length characters into the output buffer.
   * Only null-terminates if there is enough space in the buffer.
   *
   * \param buffer The buffer into which the string will be copied.
   * \param start The starting position within the string at which
   * copying begins.
   * \param length The number of characters to copy from the string.  For
   *    WriteUtf8 the number of bytes in the buffer.
   * \param nchars_ref The number of characters written, can be NULL.
   * \param options Various options that might affect performance of this or
   *    subsequent operations.
   * \return The number of characters copied to the buffer excluding the null
   *    terminator.  For WriteUtf8: The number of bytes copied to the buffer
   *    including the null terminator (if written).
   */
  enum WriteOptions {
    NO_OPTIONS = 0,
    HINT_MANY_WRITES_EXPECTED = 1,
    NO_NULL_TERMINATION = 2,
    PRESERVE_ASCII_NULL = 4
  };

  uint16_t GetCharacter(int index);

  // 16-bit character codes.
  int Write(uint16_t* buffer,
            int start = 0,
            int length = -1,
            int options = NO_OPTIONS) const;

  /**
   * A zero length string.
   */
  static v8::Local<v8::String> Empty();
  static v8::Local<v8::String> Empty(Isolate* isolate);

  /**
   * Returns true if the string is external
   */
  bool IsExternal() const;

  class V8EXPORT ExternalStringResourceBase {  // NOLINT
   public:
    virtual ~ExternalStringResourceBase() {}

   protected:
    ExternalStringResourceBase() {}

    /**
     * Internally V8 will call this Dispose method when the external string
     * resource is no longer needed. The default implementation will use the
     * delete operator. This method can be overridden in subclasses to
     * control how allocated external string resources are disposed.
     */
    virtual void Dispose() { delete this; }

  };

  /**
   * An ExternalStringResource is a wrapper around a two-byte string
   * buffer that resides outside V8's heap. Implement an
   * ExternalStringResource to manage the life cycle of the underlying
   * buffer.  Note that the string data must be immutable.
   */
  class V8EXPORT ExternalStringResource
      : public ExternalStringResourceBase {
   public:
    /**
     * Override the destructor to manage the life cycle of the underlying
     * buffer.
     */
    virtual ~ExternalStringResource() {}

    /**
     * The string data from the underlying buffer.
     */
    virtual const uint16_t* data() const = 0;

    /**
     * The length of the string. That is, the number of two-byte characters.
     */
    virtual size_t length() const = 0;

   protected:
    ExternalStringResource() {}
  };

  /**
   * Get the ExternalStringResource for an external string.  Returns
   * NULL if IsExternal() doesn't return true.
   */
  ExternalStringResource* GetExternalStringResource() const;

  static String* Cast(v8::Value* obj);

  /**
   * Allocates a new string from either UTF-8 encoded or ASCII data.
   * The second parameter 'length' gives the buffer length.
   * If the data is UTF-8 encoded, the caller must
   * be careful to supply the length parameter.
   * If it is not given, the function calls
   * 'strlen' to determine the buffer length, it might be
   * wrong if 'data' contains a null character.
   */
  static Local<String> New(const char* data, int length = -1);

  /** Allocates a new string from 16-bit character codes.*/
  static Local<String> New(const uint16_t* data, int length = -1);

  /** Creates a symbol. Returns one if it exists already.*/
  static Local<String> NewSymbol(const char* data, int length = -1);

  static Local<String> New(QQmlJS::VM::String *s);

  /**
   * Creates a new external string using the data defined in the given
   * resource. When the external string is no longer live on V8's heap the
   * resource will be disposed by calling its Dispose method. The caller of
   * this function should not otherwise delete or modify the resource. Neither
   * should the underlying buffer be deallocated or modified except through the
   * destructor of the external string resource.
   */
  static Local<String> NewExternal(ExternalStringResource* resource);

  /**
   * Converts an object to an ASCII string.
   * Useful if you want to print the object.
   * If conversion to a string fails (eg. due to an exception in the toString()
   * method of the object) then the length() method returns 0 and the * operator
   * returns NULL.
   */
  class V8EXPORT AsciiValue {
   public:
    explicit AsciiValue(Handle<v8::Value> obj);
    ~AsciiValue() {}
    char* operator*() { return str.data(); }
    const char* operator*() const { return str.constData(); }
    int length() const { return str.length(); }
   private:
    QByteArray str;

    // Disallow copying and assigning.
    AsciiValue(const AsciiValue&);
    void operator=(const AsciiValue&);
  };

  /**
   * Converts an object to a two-byte string.
   * If conversion to a string fails (eg. due to an exception in the toString()
   * method of the object) then the length() method returns 0 and the * operator
   * returns NULL.
   */
  class V8EXPORT Value {
   public:
    explicit Value(Handle<v8::Value> obj);
    ~Value() {}
    uint16_t* operator*() { return (uint16_t *)str.data(); }
    const uint16_t* operator*() const { return str.utf16(); }
    int length() const { return str.length(); }
   private:
    QString str;

    // Disallow copying and assigning.
    Value(const Value&);
    void operator=(const Value&);
  };

      QString asQString() const;
      QQmlJS::VM::String *asVMString() const;
};


/**
 * A JavaScript number value (ECMA-262, 4.3.20)
 */
class V8EXPORT Number : public Primitive {
 public:
  double Value() const;
  static Local<Number> New(double value);
  static Number* Cast(v8::Value* obj);
};


/**
 * A JavaScript value representing a signed integer.
 */
class V8EXPORT Integer : public Number {
 public:
  static Local<Integer> New(int32_t value);
  static Local<Integer> NewFromUnsigned(uint32_t value);
  static Local<Integer> New(int32_t value, Isolate*);
  static Local<Integer> NewFromUnsigned(uint32_t value, Isolate*);
  int64_t Value() const;
  static Integer* Cast(v8::Value* obj);
};


/**
 * A JavaScript value representing a 32-bit signed integer.
 */
class V8EXPORT Int32 : public Integer {
 public:
  int32_t Value() const;
 private:
  Int32();
};


/**
 * A JavaScript value representing a 32-bit unsigned integer.
 */
class V8EXPORT Uint32 : public Integer {
 public:
  uint32_t Value() const;
 private:
  Uint32();
};


enum PropertyAttribute {
  None       = 0,
  ReadOnly   = 1 << 0,
  DontEnum   = 1 << 1,
  DontDelete = 1 << 2
};

/**
 * Accessor[Getter|Setter] are used as callback functions when
 * setting|getting a particular property. See Object and ObjectTemplate's
 * method SetAccessor.
 */
typedef Handle<Value> (*AccessorGetter)(Local<String> property,
                                        const AccessorInfo& info);


typedef void (*AccessorSetter)(Local<String> property,
                               Local<Value> value,
                               const AccessorInfo& info);


/**
 * Access control specifications.
 *
 * Some accessors should be accessible across contexts.  These
 * accessors have an explicit access control parameter which specifies
 * the kind of cross-context access that should be allowed.
 *
 * Additionally, for security, accessors can prohibit overwriting by
 * accessors defined in JavaScript.  For objects that have such
 * accessors either locally or in their prototype chain it is not
 * possible to overwrite the accessor by using __defineGetter__ or
 * __defineSetter__ from JavaScript code.
 */
enum AccessControl {
  DEFAULT               = 0,
  ALL_CAN_READ          = 1,
  ALL_CAN_WRITE         = 1 << 1,
  PROHIBITS_OVERWRITING = 1 << 2
};


/**
 * A JavaScript object (ECMA-262, 4.3.3)
 */
class V8EXPORT Object : public Value {
 public:
    bool Set(Handle<Value> key,
             Handle<Value> value,
             PropertyAttribute attribs = None);

  bool Set(uint32_t index,
                    Handle<Value> value);

  Local<Value> Get(Handle<Value> key);

  Local<Value> Get(uint32_t index);

  // TODO(1245389): Replace the type-specific versions of these
  // functions with generic ones that accept a Handle<Value> key.
  bool Has(Handle<String> key);

  bool Delete(Handle<String> key);

  bool Has(uint32_t index);

  bool Delete(uint32_t index);

  bool SetAccessor(Handle<String> name,
                   AccessorGetter getter,
                   AccessorSetter setter = 0,
                   Handle<Value> data = Handle<Value>(),
                   AccessControl settings = DEFAULT,
                   PropertyAttribute attribute = None);

  /**
   * Returns an array containing the names of the enumerable properties
   * of this object, including properties from prototype objects.  The
   * array returned by this method contains the same values as would
   * be enumerated by a for-in statement over this object.
   */
  Local<Array> GetPropertyNames();

  /**
   * This function has the same functionality as GetPropertyNames but
   * the returned array doesn't contain the names of properties from
   * prototype objects.
   */
  Local<Array> GetOwnPropertyNames();

  /**
   * Get the prototype object.  This does not skip objects marked to
   * be skipped by __proto__ and it does not consult the security
   * handler.
   */
  Local<Value> GetPrototype();

  /**
   * Set the prototype object.  This does not skip objects marked to
   * be skipped by __proto__ and it does not consult the security
   * handler.
   */
  bool SetPrototype(Handle<Value> prototype);

  /** Gets the value in an internal field. */
  Local<Value> GetInternalField(int index);
  /** Sets the value in an internal field. */
  void SetInternalField(int index, Handle<Value> value);

  class V8EXPORT ExternalResource { // NOLINT
   public:
    ExternalResource() {}
    virtual ~ExternalResource() {}

    virtual void Dispose() { delete this; }

   private:
    // Disallow copying and assigning.
    ExternalResource(const ExternalResource&);
    void operator=(const ExternalResource&);
  };

  void SetExternalResource(ExternalResource *);
  ExternalResource *GetExternalResource();

  // Testers for local properties.
  bool HasOwnProperty(Handle<String> key);

  /**
   * Returns the identity hash for this object. The current implementation
   * uses a hidden property on the object to store the identity hash.
   *
   * The return value will never be 0. Also, it is not guaranteed to be
   * unique.
   */
  int GetIdentityHash();

  /**
   * Access hidden properties on JavaScript objects. These properties are
   * hidden from the executing JavaScript and only accessible through the V8
   * C++ API. Hidden properties introduced by V8 internally (for example the
   * identity hash) are prefixed with "v8::".
   */
  bool SetHiddenValue(Handle<String> key, Handle<Value> value);
  Local<Value> GetHiddenValue(Handle<String> key);

  /**
   * Clone this object with a fast but shallow copy.  Values will point
   * to the same values as the original object.
   */
  Local<Object> Clone();


  /**
   * Checks whether a callback is set by the
   * ObjectTemplate::SetCallAsFunctionHandler method.
   * When an Object is callable this method returns true.
   */
  bool IsCallable();

  /**
   * Call an Object as a function if a callback is set by the
   * ObjectTemplate::SetCallAsFunctionHandler method.
   */
  Local<Value> CallAsFunction(Handle<Object> recv,
                              int argc,
                              Handle<Value> argv[]);

  /**
   * Call an Object as a constructor if a callback is set by the
   * ObjectTemplate::SetCallAsFunctionHandler method.
   * Note: This method behaves like the Function::NewInstance method.
   */
  Local<Value> CallAsConstructor(int argc,
                                 Handle<Value> argv[]);

  static Local<Object> New();
  static Object* Cast(Value* obj);
};


/**
 * An instance of the built-in array constructor (ECMA-262, 15.4.2).
 */
class V8EXPORT Array : public Object {
 public:
  uint32_t Length() const;

  /**
   * Creates a JavaScript array with the given length. If the length
   * is negative the returned array will have length 0.
   */
  static Local<Array> New(int length = 0);

  static Array* Cast(Value* obj);
};


/**
 * A JavaScript function object (ECMA-262, 15.3).
 */
class V8EXPORT Function : public Object {
 public:
  Local<Object> NewInstance() const;
  Local<Object> NewInstance(int argc, Handle<Value> argv[]) const;
  Local<Value> Call(Handle<Object> recv,
                    int argc,
                    Handle<Value> argv[]);
  Handle<Value> GetName() const;

  ScriptOrigin GetScriptOrigin() const;
  static Function* Cast(Value* obj);
};


/**
 * An instance of the built-in Date constructor (ECMA-262, 15.9).
 */
class V8EXPORT Date : public Object {
 public:
  static Local<Value> New(double time);

  /**
   * A specialization of Value::NumberValue that is more efficient
   * because we know the structure of this object.
   */
  double NumberValue() const;

  static Date* Cast(v8::Value* obj);

  /**
   * Notification that the embedder has changed the time zone,
   * daylight savings time, or other date / time configuration
   * parameters.  V8 keeps a cache of various values used for
   * date / time computation.  This notification will reset
   * those cached values for the current context so that date /
   * time configuration changes would be reflected in the Date
   * object.
   *
   * This API should not be called more than needed as it will
   * negatively impact the performance of date operations.
   */
  static void DateTimeConfigurationChangeNotification();

};


/**
 * A Number object (ECMA-262, 4.3.21).
 */
class V8EXPORT NumberObject : public Object {
 public:
  static Local<Value> New(double value);

  /**
   * Returns the Number held by the object.
   */
  double NumberValue() const;

  static NumberObject* Cast(v8::Value* obj);

};


/**
 * A Boolean object (ECMA-262, 4.3.15).
 */
class V8EXPORT BooleanObject : public Object {
 public:
  static Local<Value> New(bool value);

  /**
   * Returns the Boolean held by the object.
   */
  bool BooleanValue() const;

  static BooleanObject* Cast(v8::Value* obj);

};


/**
 * A String object (ECMA-262, 4.3.18).
 */
class V8EXPORT StringObject : public Object {
 public:
  static Local<Value> New(Handle<String> value);

  /**
   * Returns the String held by the object.
   */
  Local<String> StringValue() const;

  static StringObject* Cast(v8::Value* obj);

};


/**
 * An instance of the built-in RegExp constructor (ECMA-262, 15.10).
 */
class V8EXPORT RegExp : public Object {
 public:
  /**
   * Regular expression flag bits. They can be or'ed to enable a set
   * of flags.
   */
  enum Flags {
    kNone = 0,
    kGlobal = 1,
    kIgnoreCase = 2,
    kMultiline = 4
  };

  /**
   * Creates a regular expression from the given pattern string and
   * the flags bit field. May throw a JavaScript exception as
   * described in ECMA-262, 15.10.4.1.
   *
   * For example,
   *   RegExp::New(v8::String::New("foo"),
   *               static_cast<RegExp::Flags>(kGlobal | kMultiline))
   * is equivalent to evaluating "/foo/gm".
   */
  static Local<RegExp> New(Handle<String> pattern,
                           Flags flags);

  /**
   * Returns the value of the source property: a string representing
   * the regular expression.
   */
  Local<String> GetSource() const;

  /**
   * Returns the flags bit field.
   */
  Flags GetFlags() const;

  static RegExp* Cast(v8::Value* obj);

};


/**
 * A JavaScript value that wraps a C++ void*.  This type of value is
 * mainly used to associate C++ data structures with JavaScript
 * objects.
 *
 * The Wrap function V8 will return the most optimal Value object wrapping the
 * C++ void*. The type of the value is not guaranteed to be an External object
 * and no assumptions about its type should be made. To access the wrapped
 * value Unwrap should be used, all other operations on that object will lead
 * to unpredictable results.
 */
class V8EXPORT External : public Value {
 public:
  static Local<Value> Wrap(void* data);
  static void* Unwrap(Handle<Value> obj);

  static Local<External> New(void* value);
  static External* Cast(Value* obj);
  void* Value() const;
};


// --- Templates ---


/**
 * The superclass of object and function templates.
 */
class V8EXPORT Template : public Data {
 public:
  /** Adds a property to each instance created by this template.*/
  void Set(Handle<String> name, Handle<Value> value,
           PropertyAttribute attributes = None);
  void Set(const char* name, Handle<Value> value);

  struct Property {
      Persistent<String> name;
      Persistent<Value> value;
      PropertyAttribute attributes;
  };
  QVector<Property> m_properties;
 };

DEFINE_REFCOUNTED_HANDLE_OPERATIONS(Template)

/**
 * The argument information given to function call callbacks.  This
 * class provides access to information about the context of the call,
 * including the receiver, the number and values of arguments, and
 * the holder of the function.
 */
class V8EXPORT Arguments {
 public:
    Arguments(const QQmlJS::VM::Value *args, int argc, const QQmlJS::VM::Value &thisObject, bool isConstructor,
              const Persistent<Value> &data);
  int Length() const;
  Local<Value> operator[](int i) const;
  Local<Object> This() const;
  Local<Object> Holder() const;
  bool IsConstructCall() const;
  Local<Value> Data() const;
  Isolate* GetIsolate() const;

private:
  QVector<Persistent<Value> > m_args;
  Persistent<Object> m_thisObject;
  bool m_isConstructor;
  Persistent<Value> m_data;
};


/**
 * The information passed to an accessor callback about the context
 * of the property access.
 */
class V8EXPORT AccessorInfo {
 public:
  AccessorInfo(const QQmlJS::VM::Value &thisObject, const Persistent<Value> &data);
  Isolate* GetIsolate() const;
  Local<Value> Data() const;
  Local<Object> This() const;
  Local<Object> Holder() const;
private:
  Persistent<Value> m_this;
  Persistent<Value> m_data;
};


typedef Handle<Value> (*InvocationCallback)(const Arguments& args);

/**
 * NamedProperty[Getter|Setter] are used as interceptors on object.
 * See ObjectTemplate::SetNamedPropertyHandler.
 */
typedef Handle<Value> (*NamedPropertyGetter)(Local<String> property,
                                             const AccessorInfo& info);


/**
 * Returns the value if the setter intercepts the request.
 * Otherwise, returns an empty handle.
 */
typedef Handle<Value> (*NamedPropertySetter)(Local<String> property,
                                             Local<Value> value,
                                             const AccessorInfo& info);

/**
 * Returns a non-empty handle if the interceptor intercepts the request.
 * The result is an integer encoding property attributes (like v8::None,
 * v8::DontEnum, etc.)
 */
typedef Handle<Integer> (*NamedPropertyQuery)(Local<String> property,
                                              const AccessorInfo& info);


/**
 * Returns a non-empty handle if the deleter intercepts the request.
 * The return value is true if the property could be deleted and false
 * otherwise.
 */
typedef Handle<Boolean> (*NamedPropertyDeleter)(Local<String> property,
                                                const AccessorInfo& info);

/**
 * Returns an array containing the names of the properties the named
 * property getter intercepts.
 */
typedef Handle<Array> (*NamedPropertyEnumerator)(const AccessorInfo& info);


/**
 * Returns the value of the property if the getter intercepts the
 * request.  Otherwise, returns an empty handle.
 */
typedef Handle<Value> (*IndexedPropertyGetter)(uint32_t index,
                                               const AccessorInfo& info);


/**
 * Returns the value if the setter intercepts the request.
 * Otherwise, returns an empty handle.
 */
typedef Handle<Value> (*IndexedPropertySetter)(uint32_t index,
                                               Local<Value> value,
                                               const AccessorInfo& info);


/**
 * Returns a non-empty handle if the interceptor intercepts the request.
 * The result is an integer encoding property attributes.
 */
typedef Handle<Integer> (*IndexedPropertyQuery)(uint32_t index,
                                                const AccessorInfo& info);

/**
 * Returns a non-empty handle if the deleter intercepts the request.
 * The return value is true if the property could be deleted and false
 * otherwise.
 */
typedef Handle<Boolean> (*IndexedPropertyDeleter)(uint32_t index,
                                                  const AccessorInfo& info);

/**
 * Returns an array containing the indices of the properties the
 * indexed property getter intercepts.
 */
typedef Handle<Array> (*IndexedPropertyEnumerator)(const AccessorInfo& info);


/**
 * A FunctionTemplate is used to create functions at runtime. There
 * can only be one function created from a FunctionTemplate in a
 * context.  The lifetime of the created function is equal to the
 * lifetime of the context.  So in case the embedder needs to create
 * temporary functions that can be collected using Scripts is
 * preferred.
 *
 * A FunctionTemplate can have properties, these properties are added to the
 * function object when it is created.
 *
 * A FunctionTemplate has a corresponding instance template which is
 * used to create object instances when the function is used as a
 * constructor. Properties added to the instance template are added to
 * each object instance.
 *
 * A FunctionTemplate can have a prototype template. The prototype template
 * is used to create the prototype object of the function.
 *
 * The following example shows how to use a FunctionTemplate:
 *
 * \code
 *    v8::Local<v8::FunctionTemplate> t = v8::FunctionTemplate::New();
 *    t->Set("func_property", v8::Number::New(1));
 *
 *    v8::Local<v8::Template> proto_t = t->PrototypeTemplate();
 *    proto_t->Set("proto_method", v8::FunctionTemplate::New(InvokeCallback));
 *    proto_t->Set("proto_const", v8::Number::New(2));
 *
 *    v8::Local<v8::ObjectTemplate> instance_t = t->InstanceTemplate();
 *    instance_t->SetAccessor("instance_accessor", InstanceAccessorCallback);
 *    instance_t->SetNamedPropertyHandler(PropertyHandlerCallback, ...);
 *    instance_t->Set("instance_property", Number::New(3));
 *
 *    v8::Local<v8::Function> function = t->GetFunction();
 *    v8::Local<v8::Object> instance = function->NewInstance();
 * \endcode
 *
 * Let's use "function" as the JS variable name of the function object
 * and "instance" for the instance object created above.  The function
 * and the instance will have the following properties:
 *
 * \code
 *   func_property in function == true;
 *   function.func_property == 1;
 *
 *   function.prototype.proto_method() invokes 'InvokeCallback'
 *   function.prototype.proto_const == 2;
 *
 *   instance instanceof function == true;
 *   instance.instance_accessor calls 'InstanceAccessorCallback'
 *   instance.instance_property == 3;
 * \endcode
 *
 * A FunctionTemplate can inherit from another one by calling the
 * FunctionTemplate::Inherit method.  The following graph illustrates
 * the semantics of inheritance:
 *
 * \code
 *   FunctionTemplate Parent  -> Parent() . prototype -> { }
 *     ^                                                  ^
 *     | Inherit(Parent)                                  | .__proto__
 *     |                                                  |
 *   FunctionTemplate Child   -> Child()  . prototype -> { }
 * \endcode
 *
 * A FunctionTemplate 'Child' inherits from 'Parent', the prototype
 * object of the Child() function has __proto__ pointing to the
 * Parent() function's prototype object. An instance of the Child
 * function has all properties on Parent's instance templates.
 *
 * Let Parent be the FunctionTemplate initialized in the previous
 * section and create a Child FunctionTemplate by:
 *
 * \code
 *   Local<FunctionTemplate> parent = t;
 *   Local<FunctionTemplate> child = FunctionTemplate::New();
 *   child->Inherit(parent);
 *
 *   Local<Function> child_function = child->GetFunction();
 *   Local<Object> child_instance = child_function->NewInstance();
 * \endcode
 *
 * The Child function and Child instance will have the following
 * properties:
 *
 * \code
 *   child_func.prototype.__proto__ == function.prototype;
 *   child_instance.instance_accessor calls 'InstanceAccessorCallback'
 *   child_instance.instance_property == 3;
 * \endcode
 */
class V8EXPORT FunctionTemplate : public Template {
 public:
  /** Creates a function template.*/
  static Local<FunctionTemplate> New(
      InvocationCallback callback = 0,
      Handle<Value> data = Handle<Value>());
  /** Returns the unique function instance in the current execution context.*/
  Local<Function> GetFunction();

  /** Get the InstanceTemplate. */
  Local<ObjectTemplate> InstanceTemplate();

  /**
   * A PrototypeTemplate is the template used to create the prototype object
   * of the function created by this template.
   */
  Local<ObjectTemplate> PrototypeTemplate();

private:
  FunctionTemplate(InvocationCallback callback, Handle<Value> data);
  friend class V4V8Function;
  InvocationCallback m_callback;
  Persistent<Value> m_data;
  Local<ObjectTemplate> m_instanceTemplate;
  Local<ObjectTemplate> m_prototypeTemplate;
};

DEFINE_REFCOUNTED_HANDLE_OPERATIONS(FunctionTemplate)


/**
 * An ObjectTemplate is used to create objects at runtime.
 *
 * Properties added to an ObjectTemplate are added to each object
 * created from the ObjectTemplate.
 */
class V8EXPORT ObjectTemplate : public Template {
 public:
  /** Creates an ObjectTemplate. */
  static Local<ObjectTemplate> New();

  /** Creates a new instance of this template.*/
  Local<Object> NewInstance();

  /**
   * Sets an accessor on the object template.
   *
   * Whenever the property with the given name is accessed on objects
   * created from this ObjectTemplate the getter and setter callbacks
   * are called instead of getting and setting the property directly
   * on the JavaScript object.
   *
   * \param name The name of the property for which an accessor is added.
   * \param getter The callback to invoke when getting the property.
   * \param setter The callback to invoke when setting the property.
   * \param data A piece of data that will be passed to the getter and setter
   *   callbacks whenever they are invoked.
   * \param settings Access control settings for the accessor. This is a bit
   *   field consisting of one of more of
   *   DEFAULT = 0, ALL_CAN_READ = 1, or ALL_CAN_WRITE = 2.
   *   The default is to not allow cross-context access.
   *   ALL_CAN_READ means that all cross-context reads are allowed.
   *   ALL_CAN_WRITE means that all cross-context writes are allowed.
   *   The combination ALL_CAN_READ | ALL_CAN_WRITE can be used to allow all
   *   cross-context access.
   * \param attribute The attributes of the property for which an accessor
   *   is added.
   * \param signature The signature describes valid receivers for the accessor
   *   and is used to perform implicit instance checks against them. If the
   *   receiver is incompatible (i.e. is not an instance of the constructor as
   *   defined by FunctionTemplate::HasInstance()), an implicit TypeError is
   *   thrown and no callback is invoked.
   */
  void SetAccessor(Handle<String> name,
                   AccessorGetter getter,
                   AccessorSetter setter = 0,
                   Handle<Value> data = Handle<Value>(),
                   AccessControl settings = DEFAULT,
                   PropertyAttribute attribute = None);

  /**
   * Sets a named property handler on the object template.
   *
   * Whenever a named property is accessed on objects created from
   * this object template, the provided callback is invoked instead of
   * accessing the property directly on the JavaScript object.
   *
   * \param getter The callback to invoke when getting a property.
   * \param setter The callback to invoke when setting a property.
   * \param query The callback to invoke to check if a property is present,
   *   and if present, get its attributes.
   * \param deleter The callback to invoke when deleting a property.
   * \param enumerator The callback to invoke to enumerate all the named
   *   properties of an object.
   * \param data A piece of data that will be passed to the callbacks
   *   whenever they are invoked.
   */
  void SetNamedPropertyHandler(NamedPropertyGetter getter,
                               NamedPropertySetter setter = 0,
                               NamedPropertyQuery query = 0,
                               NamedPropertyDeleter deleter = 0,
                               NamedPropertyEnumerator enumerator = 0,
                               Handle<Value> data = Handle<Value>());
  void SetFallbackPropertyHandler(NamedPropertyGetter getter,
                                  NamedPropertySetter setter = 0,
                                  NamedPropertyQuery query = 0,
                                  NamedPropertyDeleter deleter = 0,
                                  NamedPropertyEnumerator enumerator = 0,
                                  Handle<Value> data = Handle<Value>());

  /**
   * Sets an indexed property handler on the object template.
   *
   * Whenever an indexed property is accessed on objects created from
   * this object template, the provided callback is invoked instead of
   * accessing the property directly on the JavaScript object.
   *
   * \param getter The callback to invoke when getting a property.
   * \param setter The callback to invoke when setting a property.
   * \param query The callback to invoke to check if an object has a property.
   * \param deleter The callback to invoke when deleting a property.
   * \param enumerator The callback to invoke to enumerate all the indexed
   *   properties of an object.
   * \param data A piece of data that will be passed to the callbacks
   *   whenever they are invoked.
   */
  void SetIndexedPropertyHandler(IndexedPropertyGetter getter,
                                 IndexedPropertySetter setter = 0,
                                 IndexedPropertyQuery query = 0,
                                 IndexedPropertyDeleter deleter = 0,
                                 IndexedPropertyEnumerator enumerator = 0,
                                 Handle<Value> data = Handle<Value>());

  /**
   * Gets the number of internal fields for objects generated from
   * this template.
   */
  int InternalFieldCount();

  /**
   * Sets the number of internal fields for objects generated from
   * this template.
   */
  void SetInternalFieldCount(int value);

  /**
   * Sets whether the object can store an "external resource" object.
   */
  bool HasExternalResource();
  void SetHasExternalResource(bool value);

  /**
   * Mark object instances of the template as using the user object
   * comparison callback.
   */
  void MarkAsUseUserObjectComparison();

  struct Accessor {
      Persistent<Value> getter;
      Persistent<Value> setter;
      Persistent<String> name;
      PropertyAttribute attribute;
  };

  QVector<Accessor> m_accessors;

  NamedPropertyGetter m_namedPropertyGetter;
  NamedPropertySetter m_namedPropertySetter;
  NamedPropertyQuery m_namedPropertyQuery;
  NamedPropertyDeleter m_namedPropertyDeleter;
  NamedPropertyEnumerator m_namedPropertyEnumerator;
  Persistent<Value> m_namedPropertyData;

  NamedPropertyGetter m_fallbackPropertyGetter;
  NamedPropertySetter m_fallbackPropertySetter;
  NamedPropertyQuery m_fallbackPropertyQuery;
  NamedPropertyDeleter m_fallbackPropertyDeleter;
  NamedPropertyEnumerator m_fallbackPropertyEnumerator;
  Persistent<Value> m_fallbackPropertyData;

  IndexedPropertyGetter m_indexedPropertyGetter;
  IndexedPropertySetter m_indexedPropertySetter;
  IndexedPropertyQuery m_indexedPropertyQuery;
  IndexedPropertyDeleter m_indexedPropertyDeleter;
  IndexedPropertyEnumerator m_indexedPropertyEnumerator;
  Persistent<Value> m_indexedPropertyData;

  bool m_useUserComparison;
  private:
  ObjectTemplate();
 };

DEFINE_REFCOUNTED_HANDLE_OPERATIONS(ObjectTemplate)

// --- Statics ---


Handle<Primitive> V8EXPORT Undefined();
Handle<Primitive> V8EXPORT Null();
Handle<Boolean> V8EXPORT True();
Handle<Boolean> V8EXPORT False();

inline Handle<Primitive> Undefined(Isolate*) { return Undefined(); }
inline Handle<Primitive> Null(Isolate*) { return Null(); }
inline Handle<Boolean> True(Isolate*) { return True(); }
inline Handle<Boolean> False(Isolate*) { return False(); }



// --- Exceptions ---


/**
 * Schedules an exception to be thrown when returning to JavaScript.  When an
 * exception has been scheduled it is illegal to invoke any JavaScript
 * operation; the caller must return immediately and only after the exception
 * has been handled does it become legal to invoke JavaScript operations.
 */
Handle<Value> V8EXPORT ThrowException(Handle<Value> exception);

/**
 * Create new error objects by calling the corresponding error object
 * constructor with the message.
 */
class V8EXPORT Exception {
 public:
  static Local<Value> ReferenceError(Handle<String> message);
  static Local<Value> SyntaxError(Handle<String> message);
  static Local<Value> TypeError(Handle<String> message);
  static Local<Value> Error(Handle<String> message);
};


// --- User Object Comparison Callback ---
typedef bool (*UserObjectComparisonCallback)(Local<Object> lhs,
                                             Local<Object> rhs);

// --- Garbage Collection Callbacks ---

/**
 * Applications can register callback functions which will be called
 * before and after a garbage collection.  Allocations are not
 * allowed in the callback functions, you therefore cannot manipulate
 * objects (set or delete properties for example) since it is possible
 * such operations will result in the allocation of objects.
 */
enum GCType {
  kGCTypeScavenge = 1 << 0,
  kGCTypeMarkSweepCompact = 1 << 1,
  kGCTypeAll = kGCTypeScavenge | kGCTypeMarkSweepCompact
};

enum GCCallbackFlags {
  kNoGCCallbackFlags = 0,
  kGCCallbackFlagCompacted = 1 << 0
};

typedef void (*GCPrologueCallback)(GCType type, GCCallbackFlags flags);
typedef void (*GCCallback)();



/**
 * Isolate represents an isolated instance of the V8 engine.  V8
 * isolates have completely separate states.  Objects from one isolate
 * must not be used in other isolates.  When V8 is initialized a
 * default isolate is implicitly created and entered.  The embedder
 * can create additional isolates and use them in parallel in multiple
 * threads.  An isolate can be entered by at most one thread at any
 * given time.  The Locker/Unlocker API must be used to synchronize.
 */
class V8EXPORT Isolate {
 public:
    Isolate();
    ~Isolate();
  /**
   * Stack-allocated class which sets the isolate for all operations
   * executed within a local scope.
   */
  class V8EXPORT Scope {
   public:
    explicit Scope(Isolate* isolate) : isolate_(isolate) {
      isolate->Enter();
    }

    ~Scope() { isolate_->Exit(); }

   private:
    Isolate* const isolate_;

    // Prevent copying of Scope objects.
    Scope(const Scope&);
    Scope& operator=(const Scope&);
  };

  /**
   * Creates a new isolate.  Does not change the currently entered
   * isolate.
   *
   * When an isolate is no longer used its resources should be freed
   * by calling Dispose().  Using the delete operator is not allowed.
   */
  static Isolate* New();

  /**
   * Returns the entered isolate for the current thread or NULL in
   * case there is no current isolate.
   */
  static Isolate* GetCurrent();

  /**
   * Methods below this point require holding a lock (using Locker) in
   * a multi-threaded environment.
   */

  /**
   * Sets this isolate as the entered one for the current thread.
   * Saves the previously entered one (if any), so that it can be
   * restored when exiting.  Re-entering an isolate is allowed.
   */
  void Enter();

  /**
   * Exits this isolate by restoring the previously entered one in the
   * current thread.  The isolate may still stay the same, if it was
   * entered more than once.
   *
   * Requires: this == Isolate::GetCurrent().
   */
  void Exit();

  /**
   * Disposes the isolate.  The isolate must not be entered by any
   * thread to be disposable.
   */
  void Dispose();

  /**
   * Associate embedder-specific data with the isolate
   */
  void SetData(void* data);

  /**
   * Retrieve embedder-specific data from the isolate.
   * Returns NULL if SetData has never been called.
   */
  void* GetData();

  Context *GetCurrentContext() { return m_contextStack.top(); }
  void setException(const QQmlJS::VM::Value &ex);

  private:
      friend class Context;
      friend class TryCatch;
      Isolate* m_lastIsolate;
      QStack<Context*> m_contextStack;
      TryCatch *tryCatch;
};


/**
 * Container class for static utility functions.
 */
class V8EXPORT V8 {
 public:

  /**
   * Sets V8 flags from a string.
   */
  static void SetFlagsFromString(const char* str, int length);

  /** Callback for user object comparisons */
  static void SetUserObjectComparisonCallbackFunction(UserObjectComparisonCallback);

  /**
   * Enables the host application to receive a notification before a
   * garbage collection.  Allocations are not allowed in the
   * callback function, you therefore cannot manipulate objects (set
   * or delete properties for example) since it is possible such
   * operations will result in the allocation of objects. It is possible
   * to specify the GCType filter for your callback. But it is not possible to
   * register the same callback function two times with different
   * GCType filters.
   */
  static void AddGCPrologueCallback(
      GCPrologueCallback callback, GCType gc_type_filter = kGCTypeAll);

  /**
   * This function removes callback which was installed by
   * AddGCPrologueCallback function.
   */
  static void RemoveGCPrologueCallback(GCPrologueCallback callback);

  /**
   * Allows the host application to declare implicit references between
   * the objects: if |parent| is alive, all |children| are alive too.
   * After each garbage collection, all implicit references
   * are removed.  It is intended to be used in the before-garbage-collection
   * callback function.
   */
  static void AddImplicitReferences(Persistent<Object> parent,
                                    Persistent<Value>* children,
                                    size_t length);

  /**
   * Initializes from snapshot if possible. Otherwise, attempts to
   * initialize from scratch.  This function is called implicitly if
   * you use the API without calling it first.
   */
  static bool Initialize();

  /**
   * Releases any resources used by v8 and stops any utility threads
   * that may be running.  Note that disposing v8 is permanent, it
   * cannot be reinitialized.
   *
   * It should generally not be necessary to dispose v8 before exiting
   * a process, this should happen automatically.  It is only necessary
   * to use if the process needs the resources taken up by v8.
   */
  static bool Dispose();

  /**
   * Optional notification that the embedder is idle.
   * V8 uses the notification to reduce memory footprint.
   * This call can be used repeatedly if the embedder remains idle.
   * Returns true if the embedder should stop calling IdleNotification
   * until real work has been done.  This indicates that V8 has done
   * as much cleanup as it will be able to do.
   *
   * The hint argument specifies the amount of work to be done in the function
   * on scale from 1 to 1000. There is no guarantee that the actual work will
   * match the hint.
   */
  static bool IdleNotification(int hint = 1000);

  /**
   * Optional notification that the system is running low on memory.
   * V8 uses these notifications to attempt to free memory.
   */
  static void LowMemoryNotification();
};

/**
 * An external exception handler.
 */
class V8EXPORT TryCatch {
 public:
  /**
   * Creates a new try/catch block and registers it with v8.
   */
  TryCatch();

  /**
   * Unregisters and deletes this try/catch block.
   */
  ~TryCatch();

  /**
   * Returns true if an exception has been caught by this try/catch block.
   */
  bool HasCaught() const;

  /**
   * Throws the exception caught by this TryCatch in a way that avoids
   * it being caught again by this same TryCatch.  As with ThrowException
   * it is illegal to execute any JavaScript operations after calling
   * ReThrow; the caller must return immediately to where the exception
   * is caught.
   */
  Handle<Value> ReThrow();

  /**
   * Returns the exception caught by this try/catch block.  If no exception has
   * been caught an empty handle is returned.
   *
   * The returned handle is valid until this TryCatch block has been destroyed.
   */
  Local<Value> Exception() const;

  /**
   * Returns the message associated with this exception.  If there is
   * no message associated an empty handle is returned.
   *
   * The returned handle is valid until this TryCatch block has been
   * destroyed.
   */
  Local<v8::Message> Message() const;

  /**
   * Clears any exceptions that may have been caught by this try/catch block.
   * After this method has been called, HasCaught() will return false.
   *
   * It is not necessary to clear a try/catch block before using it again; if
   * another exception is thrown the previously caught exception will just be
   * overwritten.  However, it is often a good idea since it makes it easier
   * to determine which operation threw a given exception.
   */
  void Reset();

private:
    friend class Isolate;
    TryCatch *parent;
    bool hasCaughtException;
    Local<Value> exception;
};


// --- Context ---
class V8EXPORT ExtensionConfiguration;

/**
 * A sandboxed execution context with its own set of built-in objects
 * and functions.
 */
class V8EXPORT Context : public QSharedData {
 public:
    Context();
    ~Context();

    static Local<Context> Adopt(Context *p)
    {
        Local<Context> l;
        l.object = p;
        l.object->ref.ref();
        return l;
    }
  /**
   * Returns the global proxy object or global object itself for
   * detached contexts.
   *
   * Global proxy object is a thin wrapper whose prototype points to
   * actual context's global object with the properties like Object, etc.
   * This is done that way for security reasons (for more details see
   * https://wiki.mozilla.org/Gecko:SplitWindow).
   *
   * Please note that changes to global proxy object prototype most probably
   * would break VM---v8 expects only global object as a prototype of
   * global proxy object.
   *
   * If DetachGlobal() has been invoked, Global() would return actual global
   * object until global is reattached with ReattachGlobal().
   */
  Local<Object> Global();

  /** Creates a new context.
   *
   * Returns a persistent handle to the newly allocated context. This
   * persistent handle has to be disposed when the context is no
   * longer used so the context can be garbage collected.
   *
   * \param extensions An optional extension configuration containing
   * the extensions to be installed in the newly created context.
   *
   * \param global_template An optional object template from which the
   * global object for the newly created context will be created.
   *
   * \param global_object An optional global object to be reused for
   * the newly created context. This global object must have been
   * created by a previous call to Context::New with the same global
   * template. The state of the global object will be completely reset
   * and only object identify will remain.
   */
  static Persistent<Context> New(
      ExtensionConfiguration* extensions = NULL,
      Handle<ObjectTemplate> global_template = Handle<ObjectTemplate>(),
      Handle<Value> global_object = Handle<Value>());

  /** Returns the context that is on the top of the stack. */
  static Local<Context> GetCurrent();

  /**
   * Returns the context of the calling JavaScript code.  That is the
   * context of the top-most JavaScript frame.  If there are no
   * JavaScript frames an empty handle is returned.
   */
  static Local<Context> GetCalling();
  static Local<Object> GetCallingQmlGlobal();
  static Local<Value> GetCallingScriptData();

  /**
   * Enter this context.  After entering a context, all code compiled
   * and run is compiled and run in this context.  If another context
   * is already entered, this old context is saved so it can be
   * restored when the new context is exited.
   */
  void Enter();

  /**
   * Exit this context.  Exiting the current context restores the
   * context that was in place when entering the current context.
   */
  void Exit();

  /**
   * Associate an additional data object with the context. This is mainly used
   * with the debugger to provide additional information on the context through
   * the debugger API.
   */
  void SetData(Handle<Value> data);
  Local<Value> GetData();

  /**
   * Stack-allocated class which sets the execution context for all
   * operations executed within a local scope.
   */
  class Scope {
   public:
    explicit Scope(Handle<Context> context) : context_(context) {
      context_->Enter();
    }
    ~Scope() { context_->Exit(); }
   private:
    Handle<Context> context_;
  };

  QQmlJS::VM::ExecutionEngine *GetEngine();

private:
  Context* m_lastContext;
  struct Private;
  Private *d;
  friend class Value;
  friend class Script;
  friend class Object;
  friend class Function;
};

DEFINE_REFCOUNTED_HANDLE_OPERATIONS(Context)

template<typename T>
void Persistent<T>::MakeWeak(void* parameters, WeakReferenceCallback callback)
{
    Q_UNUSED(parameters);
    Q_UNUSED(callback);

    Q_UNIMPLEMENTED();
}



}  // namespace v8


#undef V8EXPORT
#undef TYPE_CHECK


#endif  // V8_H_
