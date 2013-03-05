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
#include "qv4managed.h"
#include "qv4mm.h"
#include "qv4string.h"

namespace QQmlJS {
namespace VM {
struct Value;
struct String;
struct ExecutionEngine;
struct Object;
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
template <class T> class Handle;
template <class T> class Local;
template <class T> class Persistent;
class FunctionTemplate;
class ObjectTemplate;
class Data;
class AccessorInfo;
class StackTrace;
class StackFrame;
class Isolate;

namespace internal {

class Arguments;
class Object;
class Heap;
class HeapObject;
class Isolate;
}

void gcProtect(void *handle);
void gcUnprotect(void *handle);

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

    static void protect(Handle<T> *handle)
    {
        gcProtect(handle);
    }

    static void unProtect(Handle<T> *handle)
    {
        gcUnprotect(handle);
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
        static void protect(Handle<Type> *) {} \
        static void unProtect(Handle<Type> *) {} \
        static bool isEmpty(const Handle<Type> *handle) \
        { \
            return handle->object != 0; \
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
    T *operator*() const { return HandleOperations<T>::get(this); }

    template <typename Source>
    static Handle<T> Cast(Handle<Source> that)
    {
        return that.As<T>();
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
      : Handle<T>(reinterpret_cast<T*>(*that)) {
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
    return Local<T>(T::Cast(*that));
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
  Persistent();

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
      : Handle<T>(reinterpret_cast<T*>(*that)) {
      HandleOperations<T>::protect(this);
  }

  template <class S> Persistent(S* that) : Handle<T>(that)
  {
      HandleOperations<T>::protect(this);
  }

  /**
   * "Casts" a plain handle which is known to be a persistent handle
   * to a persistent handle.
   */
  template <class S> explicit Persistent(Handle<S> that)
      : Handle<T>(*that)
  {
      HandleOperations<T>::protect(this);
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
      return Persistent<T>(that);
  }

  /**
   * Releases the storage cell referenced by this persistent handle.
   * Does not remove the reference to the cell from any handles.
   * This handle's reference, and any other references to the storage
   * cell remain and IsEmpty will still return false.
   */
  void Dispose() {
       HandleOperations<T>::unProtect(this);
  }

  void Dispose(Isolate* isolate);

  /**
   * Make the reference to this object weak.  When only weak handles
   * refer to the object, the garbage collector will perform a
   * callback to the given V8::WeakReferenceCallback function, passing
   * it the object reference and the given parameters.
   */
  void MakeWeak(void* parameters, WeakReferenceCallback callback);

  /** Clears the weak reference to this object. */
  void ClearWeak();

  /**
   * Marks the reference to this object independent. Garbage collector
   * is free to ignore any object groups containing this object.
   * Weak callback for an independent handle should not
   * assume that it will be preceded by a global GC prologue callback
   * or followed by a global GC epilogue callback.
   */
  void MarkIndependent();
  void MarkIndependent(Isolate* isolate);

  /**
   * Marks the reference to this object partially dependent. Partially
   * dependent handles only depend on other partially dependent handles and
   * these dependencies are provided through object groups. It provides a way
   * to build smaller object groups for young objects that represent only a
   * subset of all external dependencies. This mark is automatically cleared
   * after each garbage collection.
   */
  void MarkPartiallyDependent();
  void MarkPartiallyDependent(Isolate* isolate);

  /** Returns true if this handle was previously marked as independent. */
  bool IsIndependent() const;
  bool IsIndependent(Isolate* isolate) const;

  /** Checks if the handle holds the only reference to an object. */
  bool IsNearDeath() const;

  /** Returns true if the handle's reference is weak.  */
  bool IsWeak() const;

  /**
   * Assigns a wrapper class ID to the handle. See RetainedObjectInfo
   * interface description in v8-profiler.h for details.
   */
  void SetWrapperClassId(uint16_t class_id);

  /**
   * Returns the class ID previously assigned to this handle or 0 if no class
   * ID was previously assigned.
   */
  uint16_t WrapperClassId() const;

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
  template <class T> Local<T> Close(Handle<T> value);

  /**
   * Counts the number of allocated handles.
   */
  static int NumberOfHandles();
};


// --- Special objects ---


/**
 * The superclass of values and API object templates.
 */
class V8EXPORT Data {
};


/**
 * Pre-compilation data that can be associated with a script.  This
 * data can be calculated for a script in advance of actually
 * compiling it, and can be stored between compilations.  When script
 * data is given to the compile method compilation will be faster.
 */
class V8EXPORT ScriptData {  // NOLINT
 public:
  virtual ~ScriptData() { }

  /**
   * Pre-compiles the specified script (context-independent).
   *
   * \param input Pointer to UTF-8 script source code.
   * \param length Length of UTF-8 script source code.
   */
  static ScriptData* PreCompile(const char* input, int length);

  /**
   * Pre-compiles the specified script (context-independent).
   *
   * NOTE: Pre-compilation using this method cannot happen on another thread
   * without using Lockers.
   *
   * \param source Script source code.
   */
  static ScriptData* PreCompile(Handle<String> source);

  /**
   * Load previous pre-compilation data.
   *
   * \param data Pointer to data returned by a call to Data() of a previous
   *   ScriptData. Ownership is not transferred.
   * \param length Length of data.
   */
  static ScriptData* New(const char* data, int length);

  /**
   * Returns the length of Data().
   */
  virtual int Length() = 0;

  /**
   * Returns a serialized representation of this ScriptData that can later be
   * passed to New(). NOTE: Serialized data is platform-dependent.
   */
  virtual const char* Data() = 0;

  /**
   * Returns true if the source code could not be parsed.
   */
  virtual bool HasError() = 0;
};


/**
 * The origin, within a file, of a script.
 */
class ScriptOrigin {
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
class V8EXPORT Message {
 public:
  Local<String> Get() const;
  Local<String> GetSourceLine() const;

  /**
   * Returns the resource name for the script from where the function causing
   * the error originates.
   */
  Handle<Value> GetScriptResourceName() const;

  /**
   * Returns the resource data for the script from where the function causing
   * the error originates.
   */
  Handle<Value> GetScriptData() const;

  /**
   * Exception stack trace. By default stack traces are not captured for
   * uncaught exceptions. SetCaptureStackTraceForUncaughtExceptions allows
   * to change this option.
   */
  Handle<StackTrace> GetStackTrace() const;

  /**
   * Returns the number, 1-based, of the line where the error occurred.
   */
  int GetLineNumber() const;

  /**
   * Returns the index within the script of the first character where
   * the error occurred.
   */
  int GetStartPosition() const;

  /**
   * Returns the index within the script of the last character where
   * the error occurred.
   */
  int GetEndPosition() const;

  /**
   * Returns the index within the line of the first character where
   * the error occurred.
   */
  int GetStartColumn() const;

  /**
   * Returns the index within the line of the last character where
   * the error occurred.
   */
  int GetEndColumn() const;

  // TODO(1245381): Print to a string instead of on a FILE.
  static void PrintCurrentStackTrace(FILE* out);

  static const int kNoLineNumberInfo = 0;
  static const int kNoColumnInfo = 0;
};


/**
 * Representation of a JavaScript stack trace. The information collected is a
 * snapshot of the execution stack and the information remains valid after
 * execution continues.
 */
class V8EXPORT StackTrace {
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
};


/**
 * A single JavaScript stack frame.
 */
class V8EXPORT StackFrame {
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

  /**
   * Returns whether or not the associated function is compiled via a call to
   * eval().
   */
  bool IsEval() const;

  /**
   * Returns whether or not the associated function is called as a
   * constructor via "new".
   */
  bool IsConstructor() const;
};


// --- Value ---


/**
 * The superclass of all JavaScript values and objects.
 */
class V8EXPORT Value : public Data {
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
  V8EXPORT bool IsTrue() const;

  /**
   * Returns true if this value is false.
   */
  V8EXPORT bool IsFalse() const;

  /**
   * Returns true if this value is an instance of the String type.
   * See ECMA-262 8.4.
   */
  bool IsString() const;

  /**
   * Returns true if this value is a function.
   */
  V8EXPORT bool IsFunction() const;

  /**
   * Returns true if this value is an array.
   */
  V8EXPORT bool IsArray() const;

  /**
   * Returns true if this value is an object.
   */
  V8EXPORT bool IsObject() const;

  /**
   * Returns true if this value is boolean.
   */
  V8EXPORT bool IsBoolean() const;

  /**
   * Returns true if this value is a number.
   */
  V8EXPORT bool IsNumber() const;

  /**
   * Returns true if this value is external.
   */
  V8EXPORT bool IsExternal() const;

  /**
   * Returns true if this value is a 32-bit signed integer.
   */
  V8EXPORT bool IsInt32() const;

  /**
   * Returns true if this value is a 32-bit unsigned integer.
   */
  V8EXPORT bool IsUint32() const;

  /**
   * Returns true if this value is a Date.
   */
  V8EXPORT bool IsDate() const;

  /**
   * Returns true if this value is a Boolean object.
   */
  V8EXPORT bool IsBooleanObject() const;

  /**
   * Returns true if this value is a Number object.
   */
  V8EXPORT bool IsNumberObject() const;

  /**
   * Returns true if this value is a String object.
   */
  V8EXPORT bool IsStringObject() const;

  /**
   * Returns true if this value is a NativeError.
   */
  V8EXPORT bool IsNativeError() const;

  /**
   * Returns true if this value is a RegExp.
   */
  V8EXPORT bool IsRegExp() const;

  /**
   * Returns true if this value is an Error.
   */
  V8EXPORT bool IsError() const;

  V8EXPORT Local<Boolean> ToBoolean() const;
  V8EXPORT Local<Number> ToNumber() const;
  V8EXPORT Local<String> ToString() const;
  V8EXPORT Local<String> ToDetailString() const;
  V8EXPORT Local<Object> ToObject() const;
  V8EXPORT Local<Integer> ToInteger() const;
  V8EXPORT Local<Uint32> ToUint32() const;
  V8EXPORT Local<Int32> ToInt32() const;

  /**
   * Attempts to convert a string to an array index.
   * Returns an empty handle if the conversion fails.
   */
  V8EXPORT Local<Uint32> ToArrayIndex() const;

  V8EXPORT bool BooleanValue() const;
  V8EXPORT double NumberValue() const;
  V8EXPORT int64_t IntegerValue() const;
  V8EXPORT uint32_t Uint32Value() const;
  V8EXPORT int32_t Int32Value() const;

  /** JS == */
  V8EXPORT bool Equals(Handle<Value> that) const;
  V8EXPORT bool StrictEquals(Handle<Value> that) const;

  static Handle<Value> NewFromInternalValue(quint64 val)
  {
      Handle<Value> res;
      res.val = val;
      return res;
  }

};


/**
 * The superclass of primitive values.  See ECMA-262 4.3.2.
 */
class Primitive : public Value { };


/**
 * A primitive boolean value (ECMA-262, 4.3.14).  Either the true
 * or false value.
 */
class Boolean : public Primitive {
 public:
  V8EXPORT bool Value() const;
  static Handle<Boolean> New(bool value);
};


/**
 * A JavaScript string value (ECMA-262, 4.3.17).
 */
class String : public Primitive {
 public:
  enum Encoding {
    UNKNOWN_ENCODING = 0x1,
    TWO_BYTE_ENCODING = 0x0,
    ASCII_ENCODING = 0x4
  };
  /**
   * Returns the number of characters in this string.
   */
  V8EXPORT int Length() const;

  /**
   * Returns the number of bytes in the UTF-8 encoded
   * representation of this string.
   */
  V8EXPORT int Utf8Length() const;

  /**
   * A fast conservative check for non-ASCII characters.  May
   * return true even for ASCII strings, but if it returns
   * false you can be sure that all characters are in the range
   * 0-127.
   */
  V8EXPORT bool MayContainNonAscii() const;

  /**
   * Returns the hash of this string.
   */
  V8EXPORT uint32_t Hash() const;

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
  V8EXPORT CompleteHashData CompleteHash() const;

  /**
   * Compute a hash value for the passed UTF16 string
   * data.
   */
  V8EXPORT static uint32_t ComputeHash(uint16_t *string, int length);
  V8EXPORT static uint32_t ComputeHash(char *string, int length);

  /**
   * Returns true if this string is equal to the external
   * string data provided.
   */
  V8EXPORT bool Equals(uint16_t *string, int length);
  V8EXPORT bool Equals(char *string, int length);
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

  V8EXPORT uint16_t GetCharacter(int index);

  // 16-bit character codes.
  V8EXPORT int Write(uint16_t* buffer,
                     int start = 0,
                     int length = -1,
                     int options = NO_OPTIONS) const;
  // ASCII characters.
  V8EXPORT int WriteAscii(char* buffer,
                          int start = 0,
                          int length = -1,
                          int options = NO_OPTIONS) const;
  // UTF-8 encoded characters.
  V8EXPORT int WriteUtf8(char* buffer,
                         int length = -1,
                         int* nchars_ref = NULL,
                         int options = NO_OPTIONS) const;

  /**
   * A zero length string.
   */
  V8EXPORT static v8::Local<v8::String> Empty();
  static v8::Local<v8::String> Empty(Isolate* isolate);

  /**
   * Returns true if the string is external
   */
  V8EXPORT bool IsExternal() const;

  /**
   * Returns true if the string is both external and ASCII
   */
  V8EXPORT bool IsExternalAscii() const;

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
   * An ExternalAsciiStringResource is a wrapper around an ASCII
   * string buffer that resides outside V8's heap. Implement an
   * ExternalAsciiStringResource to manage the life cycle of the
   * underlying buffer.  Note that the string data must be immutable
   * and that the data must be strict (7-bit) ASCII, not Latin-1 or
   * UTF-8, which would require special treatment internally in the
   * engine and, in the case of UTF-8, do not allow efficient indexing.
   * Use String::New or convert to 16 bit data for non-ASCII.
   */

  class V8EXPORT ExternalAsciiStringResource
      : public ExternalStringResourceBase {
   public:
    /**
     * Override the destructor to manage the life cycle of the underlying
     * buffer.
     */
    virtual ~ExternalAsciiStringResource() {}
    /** The string data from the underlying buffer.*/
    virtual const char* data() const = 0;
    /** The number of ASCII characters in the string.*/
    virtual size_t length() const = 0;
   protected:
    ExternalAsciiStringResource() {}
  };

  /**
   * If the string is an external string, return the ExternalStringResourceBase
   * regardless of the encoding, otherwise return NULL.  The encoding of the
   * string is returned in encoding_out.
   */
  ExternalStringResourceBase* GetExternalStringResourceBase(
      Encoding* encoding_out) const;

  /**
   * Get the ExternalStringResource for an external string.  Returns
   * NULL if IsExternal() doesn't return true.
   */
  ExternalStringResource* GetExternalStringResource() const;

  /**
   * Get the ExternalAsciiStringResource for an external ASCII string.
   * Returns NULL if IsExternalAscii() doesn't return true.
   */
  V8EXPORT const ExternalAsciiStringResource* GetExternalAsciiStringResource()
      const;

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
  V8EXPORT static Local<String> New(const char* data, int length = -1);

  /** Allocates a new string from 16-bit character codes.*/
  V8EXPORT static Local<String> New(const uint16_t* data, int length = -1);

  /** Creates a symbol. Returns one if it exists already.*/
  V8EXPORT static Local<String> NewSymbol(const char* data, int length = -1);

  /**
   * Creates a new string by concatenating the left and the right strings
   * passed in as parameters.
   */
  V8EXPORT static Local<String> Concat(Handle<String> left,
                                       Handle<String> right);

  /**
   * Creates a new external string using the data defined in the given
   * resource. When the external string is no longer live on V8's heap the
   * resource will be disposed by calling its Dispose method. The caller of
   * this function should not otherwise delete or modify the resource. Neither
   * should the underlying buffer be deallocated or modified except through the
   * destructor of the external string resource.
   */
  V8EXPORT static Local<String> NewExternal(ExternalStringResource* resource);

  /**
   * Associate an external string resource with this string by transforming it
   * in place so that existing references to this string in the JavaScript heap
   * will use the external string resource. The external string resource's
   * character contents need to be equivalent to this string.
   * Returns true if the string has been changed to be an external string.
   * The string is not modified if the operation fails. See NewExternal for
   * information on the lifetime of the resource.
   */
  V8EXPORT bool MakeExternal(ExternalStringResource* resource);

  /**
   * Creates a new external string using the ASCII data defined in the given
   * resource. When the external string is no longer live on V8's heap the
   * resource will be disposed by calling its Dispose method. The caller of
   * this function should not otherwise delete or modify the resource. Neither
   * should the underlying buffer be deallocated or modified except through the
   * destructor of the external string resource.
   */ V8EXPORT static Local<String> NewExternal(
      ExternalAsciiStringResource* resource);

  /**
   * Associate an external string resource with this string by transforming it
   * in place so that existing references to this string in the JavaScript heap
   * will use the external string resource. The external string resource's
   * character contents need to be equivalent to this string.
   * Returns true if the string has been changed to be an external string.
   * The string is not modified if the operation fails. See NewExternal for
   * information on the lifetime of the resource.
   */
  V8EXPORT bool MakeExternal(ExternalAsciiStringResource* resource);

  /**
   * Returns true if this string can be made external.
   */
  V8EXPORT bool CanMakeExternal();

  /** Creates an undetectable string from the supplied ASCII or UTF-8 data.*/
  V8EXPORT static Local<String> NewUndetectable(const char* data,
                                                int length = -1);

  /** Creates an undetectable string from the supplied 16-bit character codes.*/
  V8EXPORT static Local<String> NewUndetectable(const uint16_t* data,
                                                int length = -1);

  /**
   * Converts an object to a UTF-8-encoded character array.  Useful if
   * you want to print the object.  If conversion to a string fails
   * (e.g. due to an exception in the toString() method of the object)
   * then the length() method returns 0 and the * operator returns
   * NULL.
   */
  class V8EXPORT Utf8Value {
   public:
    explicit Utf8Value(Handle<v8::Value> obj);
    ~Utf8Value();
    char* operator*() { return str_; }
    const char* operator*() const { return str_; }
    int length() const { return length_; }
   private:
    char* str_;
    int length_;

    // Disallow copying and assigning.
    Utf8Value(const Utf8Value&);
    void operator=(const Utf8Value&);
  };

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
    ~Value();
    uint16_t* operator*() { return str_; }
    const uint16_t* operator*() const { return str_; }
    int length() const { return length_; }
   private:
    uint16_t* str_;
    int length_;

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
class Number : public Primitive {
 public:
  V8EXPORT double Value() const;
  V8EXPORT static Local<Number> New(double value);
  static Number* Cast(v8::Value* obj);
};


/**
 * A JavaScript value representing a signed integer.
 */
class Integer : public Number {
 public:
  V8EXPORT static Local<Integer> New(int32_t value);
  V8EXPORT static Local<Integer> NewFromUnsigned(uint32_t value);
  V8EXPORT static Local<Integer> New(int32_t value, Isolate*);
  V8EXPORT static Local<Integer> NewFromUnsigned(uint32_t value, Isolate*);
  V8EXPORT int64_t Value() const;
  static Integer* Cast(v8::Value* obj);
};


/**
 * A JavaScript value representing a 32-bit signed integer.
 */
class Int32 : public Integer {
 public:
  V8EXPORT int32_t Value() const;
 private:
  V8EXPORT Int32();
};


/**
 * A JavaScript value representing a 32-bit unsigned integer.
 */
class Uint32 : public Integer {
 public:
  V8EXPORT uint32_t Value() const;
 private:
  V8EXPORT Uint32();
};


enum PropertyAttribute {
  None       = 0,
  ReadOnly   = 1 << 0,
  DontEnum   = 1 << 1,
  DontDelete = 1 << 2
};

enum ExternalArrayType {
  kExternalByteArray = 1,
  kExternalUnsignedByteArray,
  kExternalShortArray,
  kExternalUnsignedShortArray,
  kExternalIntArray,
  kExternalUnsignedIntArray,
  kExternalFloatArray,
  kExternalDoubleArray,
  kExternalPixelArray
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
class Object : public Value {
 public:
  V8EXPORT bool Set(Handle<Value> key,
                    Handle<Value> value,
                    PropertyAttribute attribs = None);

  V8EXPORT bool Set(uint32_t index,
                    Handle<Value> value);

  // Sets a local property on this object bypassing interceptors and
  // overriding accessors or read-only properties.
  //
  // Note that if the object has an interceptor the property will be set
  // locally, but since the interceptor takes precedence the local property
  // will only be returned if the interceptor doesn't return a value.
  //
  // Note also that this only works for named properties.
  V8EXPORT bool ForceSet(Handle<Value> key,
                         Handle<Value> value,
                         PropertyAttribute attribs = None);

  V8EXPORT Local<Value> Get(Handle<Value> key);

  V8EXPORT Local<Value> Get(uint32_t index);

  /**
   * Gets the property attributes of a property which can be None or
   * any combination of ReadOnly, DontEnum and DontDelete. Returns
   * None when the property doesn't exist.
   */
  V8EXPORT PropertyAttribute GetPropertyAttributes(Handle<Value> key);

  // TODO(1245389): Replace the type-specific versions of these
  // functions with generic ones that accept a Handle<Value> key.
  V8EXPORT bool Has(Handle<String> key);

  V8EXPORT bool Delete(Handle<String> key);

  // Delete a property on this object bypassing interceptors and
  // ignoring dont-delete attributes.
  V8EXPORT bool ForceDelete(Handle<Value> key);

  V8EXPORT bool Has(uint32_t index);

  V8EXPORT bool Delete(uint32_t index);

  V8EXPORT bool SetAccessor(Handle<String> name,
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
  V8EXPORT Local<Array> GetPropertyNames();

  /**
   * This function has the same functionality as GetPropertyNames but
   * the returned array doesn't contain the names of properties from
   * prototype objects.
   */
  V8EXPORT Local<Array> GetOwnPropertyNames();

  /**
   * Get the prototype object.  This does not skip objects marked to
   * be skipped by __proto__ and it does not consult the security
   * handler.
   */
  V8EXPORT Local<Value> GetPrototype();

  /**
   * Set the prototype object.  This does not skip objects marked to
   * be skipped by __proto__ and it does not consult the security
   * handler.
   */
  V8EXPORT bool SetPrototype(Handle<Value> prototype);

  /**
   * Finds an instance of the given function template in the prototype
   * chain.
   */
  V8EXPORT Local<Object> FindInstanceInPrototypeChain(
      Handle<FunctionTemplate> tmpl);

  /**
   * Call builtin Object.prototype.toString on this object.
   * This is different from Value::ToString() that may call
   * user-defined toString function. This one does not.
   */
  V8EXPORT Local<String> ObjectProtoToString();

  /**
   * Returns the function invoked as a constructor for this object.
   * May be the null value.
   */
  V8EXPORT Local<Value> GetConstructor();

  /**
   * Returns the name of the function invoked as a constructor for this object.
   */
  V8EXPORT Local<String> GetConstructorName();

  /** Gets the number of internal fields for this Object. */
  V8EXPORT int InternalFieldCount();
  /** Gets the value in an internal field. */
  Local<Value> GetInternalField(int index);
  /** Sets the value in an internal field. */
  V8EXPORT void SetInternalField(int index, Handle<Value> value);

  /** Gets a native pointer from an internal field. */
  void* GetPointerFromInternalField(int index);

  /** Sets a native pointer in an internal field. */
  V8EXPORT void SetPointerInInternalField(int index, void* value);

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

  V8EXPORT void SetExternalResource(ExternalResource *);
  V8EXPORT ExternalResource *GetExternalResource();

  // Testers for local properties.
  V8EXPORT bool HasOwnProperty(Handle<String> key);
  V8EXPORT bool HasRealNamedProperty(Handle<String> key);
  V8EXPORT bool HasRealIndexedProperty(uint32_t index);
  V8EXPORT bool HasRealNamedCallbackProperty(Handle<String> key);

  /**
   * If result.IsEmpty() no real property was located in the prototype chain.
   * This means interceptors in the prototype chain are not called.
   */
  V8EXPORT Local<Value> GetRealNamedPropertyInPrototypeChain(
      Handle<String> key);

  /**
   * If result.IsEmpty() no real property was located on the object or
   * in the prototype chain.
   * This means interceptors in the prototype chain are not called.
   */
  V8EXPORT Local<Value> GetRealNamedProperty(Handle<String> key);

  /** Tests for a named lookup interceptor.*/
  V8EXPORT bool HasNamedLookupInterceptor();

  /** Tests for an index lookup interceptor.*/
  V8EXPORT bool HasIndexedLookupInterceptor();

  /**
   * Turns on access check on the object if the object is an instance of
   * a template that has access check callbacks. If an object has no
   * access check info, the object cannot be accessed by anyone.
   */
  V8EXPORT void TurnOnAccessCheck();

  /**
   * Returns the identity hash for this object. The current implementation
   * uses a hidden property on the object to store the identity hash.
   *
   * The return value will never be 0. Also, it is not guaranteed to be
   * unique.
   */
  V8EXPORT int GetIdentityHash();

  /**
   * Access hidden properties on JavaScript objects. These properties are
   * hidden from the executing JavaScript and only accessible through the V8
   * C++ API. Hidden properties introduced by V8 internally (for example the
   * identity hash) are prefixed with "v8::".
   */
  V8EXPORT bool SetHiddenValue(Handle<String> key, Handle<Value> value);
  V8EXPORT Local<Value> GetHiddenValue(Handle<String> key);
  V8EXPORT bool DeleteHiddenValue(Handle<String> key);

  /**
   * Returns true if this is an instance of an api function (one
   * created from a function created from a function template) and has
   * been modified since it was created.  Note that this method is
   * conservative and may return true for objects that haven't actually
   * been modified.
   */
  V8EXPORT bool IsDirty();

  /**
   * Clone this object with a fast but shallow copy.  Values will point
   * to the same values as the original object.
   */
  V8EXPORT Local<Object> Clone();

  /**
   * Returns the context in which the object was created.
   */
  V8EXPORT Local<Context> CreationContext();

  /**
   * Set the backing store of the indexed properties to be managed by the
   * embedding layer. Access to the indexed properties will follow the rules
   * spelled out in CanvasPixelArray.
   * Note: The embedding program still owns the data and needs to ensure that
   *       the backing store is preserved while V8 has a reference.
   */
  V8EXPORT void SetIndexedPropertiesToPixelData(uint8_t* data, int length);
  V8EXPORT bool HasIndexedPropertiesInPixelData();
  V8EXPORT uint8_t* GetIndexedPropertiesPixelData();
  V8EXPORT int GetIndexedPropertiesPixelDataLength();

  /**
   * Set the backing store of the indexed properties to be managed by the
   * embedding layer. Access to the indexed properties will follow the rules
   * spelled out for the CanvasArray subtypes in the WebGL specification.
   * Note: The embedding program still owns the data and needs to ensure that
   *       the backing store is preserved while V8 has a reference.
   */
  V8EXPORT void SetIndexedPropertiesToExternalArrayData(
      void* data,
      ExternalArrayType array_type,
      int number_of_elements);
  V8EXPORT bool HasIndexedPropertiesInExternalArrayData();
  V8EXPORT void* GetIndexedPropertiesExternalArrayData();
  V8EXPORT ExternalArrayType GetIndexedPropertiesExternalArrayDataType();
  V8EXPORT int GetIndexedPropertiesExternalArrayDataLength();

  /**
   * Checks whether a callback is set by the
   * ObjectTemplate::SetCallAsFunctionHandler method.
   * When an Object is callable this method returns true.
   */
  V8EXPORT bool IsCallable();

  /**
   * Call an Object as a function if a callback is set by the
   * ObjectTemplate::SetCallAsFunctionHandler method.
   */
  V8EXPORT Local<Value> CallAsFunction(Handle<Object> recv,
                                       int argc,
                                       Handle<Value> argv[]);

  /**
   * Call an Object as a constructor if a callback is set by the
   * ObjectTemplate::SetCallAsFunctionHandler method.
   * Note: This method behaves like the Function::NewInstance method.
   */
  V8EXPORT Local<Value> CallAsConstructor(int argc,
                                          Handle<Value> argv[]);

  V8EXPORT static Local<Object> New();
  static Object* Cast(Value* obj);
};


/**
 * An instance of the built-in array constructor (ECMA-262, 15.4.2).
 */
class Array : public Object {
 public:
  V8EXPORT uint32_t Length() const;

  /**
   * Clones an element at index |index|.  Returns an empty
   * handle if cloning fails (for any reason).
   */
  V8EXPORT Local<Object> CloneElementAt(uint32_t index);

  /**
   * Creates a JavaScript array with the given length. If the length
   * is negative the returned array will have length 0.
   */
  V8EXPORT static Local<Array> New(int length = 0);

  static Array* Cast(Value* obj);
};


/**
 * A JavaScript function object (ECMA-262, 15.3).
 */
class Function : public Object {
 public:
  V8EXPORT Local<Object> NewInstance() const;
  V8EXPORT Local<Object> NewInstance(int argc, Handle<Value> argv[]) const;
  V8EXPORT Local<Value> Call(Handle<Object> recv,
                             int argc,
                             Handle<Value> argv[]);
  V8EXPORT void SetName(Handle<String> name);
  V8EXPORT Handle<Value> GetName() const;

  /**
   * Name inferred from variable or property assignment of this function.
   * Used to facilitate debugging and profiling of JavaScript code written
   * in an OO style, where many functions are anonymous but are assigned
   * to object properties.
   */
  V8EXPORT Handle<Value> GetInferredName() const;

  /**
   * Returns zero based line number of function body and
   * kLineOffsetNotFound if no information available.
   */
  V8EXPORT int GetScriptLineNumber() const;
  /**
   * Returns zero based column number of function body and
   * kLineOffsetNotFound if no information available.
   */
  V8EXPORT int GetScriptColumnNumber() const;
  V8EXPORT Handle<Value> GetScriptId() const;
  V8EXPORT ScriptOrigin GetScriptOrigin() const;
  static Function* Cast(Value* obj);
  V8EXPORT static const int kLineOffsetNotFound;

};


/**
 * An instance of the built-in Date constructor (ECMA-262, 15.9).
 */
class Date : public Object {
 public:
  V8EXPORT static Local<Value> New(double time);

  /**
   * A specialization of Value::NumberValue that is more efficient
   * because we know the structure of this object.
   */
  V8EXPORT double NumberValue() const;

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
  V8EXPORT static void DateTimeConfigurationChangeNotification();

};


/**
 * A Number object (ECMA-262, 4.3.21).
 */
class NumberObject : public Object {
 public:
  V8EXPORT static Local<Value> New(double value);

  /**
   * Returns the Number held by the object.
   */
  V8EXPORT double NumberValue() const;

  static NumberObject* Cast(v8::Value* obj);

};


/**
 * A Boolean object (ECMA-262, 4.3.15).
 */
class BooleanObject : public Object {
 public:
  V8EXPORT static Local<Value> New(bool value);

  /**
   * Returns the Boolean held by the object.
   */
  V8EXPORT bool BooleanValue() const;

  static BooleanObject* Cast(v8::Value* obj);

};


/**
 * A String object (ECMA-262, 4.3.18).
 */
class StringObject : public Object {
 public:
  V8EXPORT static Local<Value> New(Handle<String> value);

  /**
   * Returns the String held by the object.
   */
  V8EXPORT Local<String> StringValue() const;

  static StringObject* Cast(v8::Value* obj);

};


/**
 * An instance of the built-in RegExp constructor (ECMA-262, 15.10).
 */
class RegExp : public Object {
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
  V8EXPORT static Local<RegExp> New(Handle<String> pattern,
                                    Flags flags);

  /**
   * Returns the value of the source property: a string representing
   * the regular expression.
   */
  V8EXPORT Local<String> GetSource() const;

  /**
   * Returns the flags bit field.
   */
  V8EXPORT Flags GetFlags() const;

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
class External : public Value {
 public:
  V8EXPORT static Local<Value> Wrap(void* data);
  static void* Unwrap(Handle<Value> obj);

  V8EXPORT static Local<External> New(void* value);
  static External* Cast(Value* obj);
  V8EXPORT void* Value() const;
};


// --- Templates ---


/**
 * The superclass of object and function templates.
 */
class V8EXPORT Template : public Data {
 public:
  /** Adds a property to each instance created by this template.*/
  void Set(Handle<String> name, Handle<Data> value,
           PropertyAttribute attributes = None);
  void Set(const char* name, Handle<Data> value);
 };


/**
 * The argument information given to function call callbacks.  This
 * class provides access to information about the context of the call,
 * including the receiver, the number and values of arguments, and
 * the holder of the function.
 */
class Arguments {
 public:
  int Length() const;
  Local<Value> operator[](int i) const;
  Local<Function> Callee() const;
  Local<Object> This() const;
  Local<Object> Holder() const;
  bool IsConstructCall() const;
  Local<Value> Data() const;
  Isolate* GetIsolate() const;

};


/**
 * The information passed to an accessor callback about the context
 * of the property access.
 */
class V8EXPORT AccessorInfo {
 public:
  AccessorInfo(internal::Object** args);
  Isolate* GetIsolate() const;
  Local<Value> Data() const;
  Local<Object> This() const;
  Local<Object> Holder() const;

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
 * Access type specification.
 */
enum AccessType {
  ACCESS_GET,
  ACCESS_SET,
  ACCESS_HAS,
  ACCESS_DELETE,
  ACCESS_KEYS
};


/**
 * Returns true if cross-context access should be allowed to the named
 * property with the given key on the host object.
 */
typedef bool (*NamedSecurityCallback)(Local<Object> host,
                                      Local<Value> key,
                                      AccessType type,
                                      Local<Value> data);


/**
 * Returns true if cross-context access should be allowed to the indexed
 * property with the given index on the host object.
 */
typedef bool (*IndexedSecurityCallback)(Local<Object> host,
                                        uint32_t index,
                                        AccessType type,
                                        Local<Value> data);


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
      Handle<Value> data = Handle<Value>(),
      Handle<Signature> signature = Handle<Signature>());
  /** Returns the unique function instance in the current execution context.*/
  Local<Function> GetFunction();

  /**
   * Set the call-handler callback for a FunctionTemplate.  This
   * callback is called whenever the function created from this
   * FunctionTemplate is called.
   */
  void SetCallHandler(InvocationCallback callback,
                      Handle<Value> data = Handle<Value>());

  /** Get the InstanceTemplate. */
  Local<ObjectTemplate> InstanceTemplate();

  /** Causes the function template to inherit from a parent function template.*/
  void Inherit(Handle<FunctionTemplate> parent);

  /**
   * A PrototypeTemplate is the template used to create the prototype object
   * of the function created by this template.
   */
  Local<ObjectTemplate> PrototypeTemplate();


  /**
   * Set the class name of the FunctionTemplate.  This is used for
   * printing objects created with the function created from the
   * FunctionTemplate as its constructor.
   */
  void SetClassName(Handle<String> name);

  /**
   * Determines whether the __proto__ accessor ignores instances of
   * the function template.  If instances of the function template are
   * ignored, __proto__ skips all instances and instead returns the
   * next object in the prototype chain.
   *
   * Call with a value of true to make the __proto__ accessor ignore
   * instances of the function template.  Call with a value of false
   * to make the __proto__ accessor not ignore instances of the
   * function template.  By default, instances of a function template
   * are not ignored.
   */
  void SetHiddenPrototype(bool value);

  /**
   * Sets the ReadOnly flag in the attributes of the 'prototype' property
   * of functions created from this FunctionTemplate to true.
   */
  void ReadOnlyPrototype();

  /**
   * Returns true if the given object is an instance of this function
   * template.
   */
  bool HasInstance(Handle<Value> object);

};


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
                   PropertyAttribute attribute = None,
                   Handle<AccessorSignature> signature =
                       Handle<AccessorSignature>());

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
   * Sets the callback to be used when calling instances created from
   * this template as a function.  If no callback is set, instances
   * behave like normal JavaScript objects that cannot be called as a
   * function.
   */
  void SetCallAsFunctionHandler(InvocationCallback callback,
                                Handle<Value> data = Handle<Value>());

  /**
   * Mark object instances of the template as undetectable.
   *
   * In many ways, undetectable objects behave as though they are not
   * there.  They behave like 'undefined' in conditionals and when
   * printed.  However, properties can be accessed and called as on
   * normal objects.
   */
  void MarkAsUndetectable();

  /**
   * Sets access check callbacks on the object template.
   *
   * When accessing properties on instances of this object template,
   * the access check callback will be called to determine whether or
   * not to allow cross-context access to the properties.
   * The last parameter specifies whether access checks are turned
   * on by default on instances. If access checks are off by default,
   * they can be turned on on individual instances by calling
   * Object::TurnOnAccessCheck().
   */
  void SetAccessCheckCallbacks(NamedSecurityCallback named_handler,
                               IndexedSecurityCallback indexed_handler,
                               Handle<Value> data = Handle<Value>(),
                               bool turned_on_by_default = true);

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

 };


/**
 * A Signature specifies which receivers and arguments are valid
 * parameters to a function.
 */
class V8EXPORT Signature : public Data {
 public:
  static Local<Signature> New(Handle<FunctionTemplate> receiver =
                                  Handle<FunctionTemplate>(),
                              int argc = 0,
                              Handle<FunctionTemplate> argv[] = 0);
 private:
  Signature();
};


/**
 * An AccessorSignature specifies which receivers are valid parameters
 * to an accessor callback.
 */
class V8EXPORT AccessorSignature : public Data {
 public:
  static Local<AccessorSignature> New(Handle<FunctionTemplate> receiver =
                                          Handle<FunctionTemplate>());
 private:
  AccessorSignature();
};


/**
 * A utility for determining the type of objects based on the template
 * they were constructed from.
 */
class V8EXPORT TypeSwitch : public Data {
 public:
  static Local<TypeSwitch> New(Handle<FunctionTemplate> type);
  static Local<TypeSwitch> New(int argc, Handle<FunctionTemplate> types[]);
  int match(Handle<Value> value);
 private:
  TypeSwitch();
};


// --- Extensions ---

class V8EXPORT ExternalAsciiStringResourceImpl
    : public String::ExternalAsciiStringResource {
 public:
  ExternalAsciiStringResourceImpl() : data_(0), length_(0) {}
  ExternalAsciiStringResourceImpl(const char* data, size_t length)
      : data_(data), length_(length) {}
  const char* data() const { return data_; }
  size_t length() const { return length_; }

 private:
  const char* data_;
  size_t length_;
};

// --- Statics ---


Handle<Primitive> V8EXPORT Undefined();
Handle<Primitive> V8EXPORT Null();
Handle<Boolean> V8EXPORT True();
Handle<Boolean> V8EXPORT False();

Handle<Primitive> Undefined(Isolate* isolate);
Handle<Primitive> Null(Isolate* isolate);
Handle<Boolean> True(Isolate* isolate);
Handle<Boolean> False(Isolate* isolate);



// --- Exceptions ---


typedef void (*FatalErrorCallback)(const char* location, const char* message);


typedef void (*MessageCallback)(Handle<Message> message, Handle<Value> error);


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
  static Local<Value> RangeError(Handle<String> message);
  static Local<Value> ReferenceError(Handle<String> message);
  static Local<Value> SyntaxError(Handle<String> message);
  static Local<Value> TypeError(Handle<String> message);
  static Local<Value> Error(Handle<String> message);
};


// --- Counters Callbacks ---

typedef int* (*CounterLookupCallback)(const char* name);

typedef void* (*CreateHistogramCallback)(const char* name,
                                         int min,
                                         int max,
                                         size_t buckets);

typedef void (*AddHistogramSampleCallback)(void* histogram, int sample);

// --- Memory Allocation Callback ---
  enum ObjectSpace {
    kObjectSpaceNewSpace = 1 << 0,
    kObjectSpaceOldPointerSpace = 1 << 1,
    kObjectSpaceOldDataSpace = 1 << 2,
    kObjectSpaceCodeSpace = 1 << 3,
    kObjectSpaceMapSpace = 1 << 4,
    kObjectSpaceLoSpace = 1 << 5,

    kObjectSpaceAll = kObjectSpaceNewSpace | kObjectSpaceOldPointerSpace |
      kObjectSpaceOldDataSpace | kObjectSpaceCodeSpace | kObjectSpaceMapSpace |
      kObjectSpaceLoSpace
  };

  enum AllocationAction {
    kAllocationActionAllocate = 1 << 0,
    kAllocationActionFree = 1 << 1,
    kAllocationActionAll = kAllocationActionAllocate | kAllocationActionFree
  };

typedef void (*MemoryAllocationCallback)(ObjectSpace space,
                                         AllocationAction action,
                                         int size);

// --- Leave Script Callback ---
typedef void (*CallCompletedCallback)();

// --- Failed Access Check Callback ---
typedef void (*FailedAccessCheckCallback)(Local<Object> target,
                                          AccessType type,
                                          Local<Value> data);

// --- User Object Comparison Callback ---
typedef bool (*UserObjectComparisonCallback)(Local<Object> lhs,
                                             Local<Object> rhs);

// --- AllowCodeGenerationFromStrings callbacks ---

/**
 * Callback to check if code generation from strings is allowed. See
 * Context::AllowCodeGenerationFromStrings.
 */
typedef bool (*AllowCodeGenerationFromStringsCallback)(Local<Context> context);

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
typedef void (*GCEpilogueCallback)(GCType type, GCCallbackFlags flags);

typedef void (*GCCallback)();


/**
 * Collection of V8 heap information.
 *
 * Instances of this class can be passed to v8::V8::HeapStatistics to
 * get heap statistics from V8.
 */
class V8EXPORT HeapStatistics {
 public:
  HeapStatistics();
  size_t total_heap_size() { return total_heap_size_; }
  size_t total_heap_size_executable() { return total_heap_size_executable_; }
  size_t total_physical_size() { return total_physical_size_; }
  size_t used_heap_size() { return used_heap_size_; }
  size_t heap_size_limit() { return heap_size_limit_; }

 private:
  void set_total_heap_size(size_t size) { total_heap_size_ = size; }
  void set_total_heap_size_executable(size_t size) {
    total_heap_size_executable_ = size;
  }
  void set_total_physical_size(size_t size) {
    total_physical_size_ = size;
  }
  void set_used_heap_size(size_t size) { used_heap_size_ = size; }
  void set_heap_size_limit(size_t size) { heap_size_limit_ = size; }

  size_t total_heap_size_;
  size_t total_heap_size_executable_;
  size_t total_physical_size_;
  size_t used_heap_size_;
  size_t heap_size_limit_;

  friend class V8;
};


class RetainedObjectInfo;

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

  Context *GetCurrentContext() { return m_currentContext; }

  private:
      friend class Context;
      Isolate* m_lastIsolate;
      Context* m_currentContext;
};

class StartupData {
 public:
  enum CompressionAlgorithm {
    kUncompressed,
    kBZip2
  };

  const char* data;
  int compressed_size;
  int raw_size;
};


/**
 * EntropySource is used as a callback function when v8 needs a source
 * of entropy.
 */
typedef bool (*EntropySource)(unsigned char* buffer, size_t length);


/**
 * ReturnAddressLocationResolver is used as a callback function when v8 is
 * resolving the location of a return address on the stack. Profilers that
 * change the return address on the stack can use this to resolve the stack
 * location to whereever the profiler stashed the original return address.
 *
 * \param return_addr_location points to a location on stack where a machine
 *    return address resides.
 * \returns either return_addr_location, or else a pointer to the profiler's
 *    copy of the original return address.
 *
 * \note the resolver function must not cause garbage collection.
 */
typedef uintptr_t (*ReturnAddressLocationResolver)(
    uintptr_t return_addr_location);


/**
 * FunctionEntryHook is the type of the profile entry hook called at entry to
 * any generated function when function-level profiling is enabled.
 *
 * \param function the address of the function that's being entered.
 * \param return_addr_location points to a location on stack where the machine
 *    return address resides. This can be used to identify the caller of
 *    \p function, and/or modified to divert execution when \p function exits.
 *
 * \note the entry hook must not cause garbage collection.
 */
typedef void (*FunctionEntryHook)(uintptr_t function,
                                  uintptr_t return_addr_location);


/**
 * A JIT code event is issued each time code is added, moved or removed.
 *
 * \note removal events are not currently issued.
 */
struct JitCodeEvent {
  enum EventType {
    CODE_ADDED,
    CODE_MOVED,
    CODE_REMOVED
  };

  // Type of event.
  EventType type;
  // Start of the instructions.
  void* code_start;
  // Size of the instructions.
  size_t code_len;

  union {
    // Only valid for CODE_ADDED.
    struct {
      // Name of the object associated with the code, note that the string is
      // not zero-terminated.
      const char* str;
      // Number of chars in str.
      size_t len;
    } name;
    // New location of instructions. Only valid for CODE_MOVED.
    void* new_code_start;
  };
};

/**
 * Option flags passed to the SetJitCodeEventHandler function.
 */
enum JitCodeEventOptions {
  kJitCodeEventDefault = 0,
  // Generate callbacks for already existent code.
  kJitCodeEventEnumExisting = 1
};


/**
 * Callback function passed to SetJitCodeEventHandler.
 *
 * \param event code add, move or removal event.
 */
typedef void (*JitCodeEventHandler)(const JitCodeEvent* event);


/**
 * Interface for iterating through all external resources in the heap.
 */
class V8EXPORT ExternalResourceVisitor {  // NOLINT
 public:
  virtual ~ExternalResourceVisitor() {}
  virtual void VisitExternalString(Handle<String>) {}
};


/**
 * Interface for iterating through all the persistent handles in the heap.
 */
class V8EXPORT PersistentHandleVisitor {  // NOLINT
 public:
  virtual ~PersistentHandleVisitor() {}
  virtual void VisitPersistentHandle(Persistent<Value>,
                                     uint16_t) {}
};


/**
 * Container class for static utility functions.
 */
class V8EXPORT V8 {
 public:
  /** Set the callback to invoke in case of fatal errors. */
  static void SetFatalErrorHandler(FatalErrorCallback that);

  /**
   * Set the callback to invoke to check if code generation from
   * strings should be allowed.
   */
  static void SetAllowCodeGenerationFromStringsCallback(
      AllowCodeGenerationFromStringsCallback that);

  /**
   * Ignore out-of-memory exceptions.
   *
   * V8 running out of memory is treated as a fatal error by default.
   * This means that the fatal error handler is called and that V8 is
   * terminated.
   *
   * IgnoreOutOfMemoryException can be used to not treat an
   * out-of-memory situation as a fatal error.  This way, the contexts
   * that did not cause the out of memory problem might be able to
   * continue execution.
   */
  static void IgnoreOutOfMemoryException();

  /**
   * Check if V8 is dead and therefore unusable.  This is the case after
   * fatal errors such as out-of-memory situations.
   */
  static bool IsDead();

  /**
   * Adds a message listener.
   *
   * The same message listener can be added more than once and in that
   * case it will be called more than once for each message.
   */
  static bool AddMessageListener(MessageCallback that);

  /**
   * Remove all message listeners from the specified callback function.
   */
  static void RemoveMessageListeners(MessageCallback that);

  /**
   * Tells V8 to capture current stack trace when uncaught exception occurs
   * and report it to the message listeners. The option is off by default.
   */
  static void SetCaptureStackTraceForUncaughtExceptions(
      bool capture,
      int frame_limit = 10,
      StackTrace::StackTraceOptions options = StackTrace::kOverview);

  /**
   * Sets V8 flags from a string.
   */
  static void SetFlagsFromString(const char* str, int length);

  /**
   * Sets V8 flags from the command line.
   */
  static void SetFlagsFromCommandLine(int* argc,
                                      char** argv,
                                      bool remove_flags);

  /** Get the version string. */
  static const char* GetVersion();

  /**
   * Enables the host application to provide a mechanism for recording
   * statistics counters.
   */
  static void SetCounterFunction(CounterLookupCallback);

  /**
   * Enables the host application to provide a mechanism for recording
   * histograms. The CreateHistogram function returns a
   * histogram which will later be passed to the AddHistogramSample
   * function.
   */
  static void SetCreateHistogramFunction(CreateHistogramCallback);
  static void SetAddHistogramSampleFunction(AddHistogramSampleCallback);

  /**
   * Enables the computation of a sliding window of states. The sliding
   * window information is recorded in statistics counters.
   */
  static void EnableSlidingStateWindow();

  /** Callback function for reporting failed access checks.*/
  static void SetFailedAccessCheckCallbackFunction(FailedAccessCheckCallback);

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
   * The function is deprecated. Please use AddGCPrologueCallback instead.
   * Enables the host application to receive a notification before a
   * garbage collection.  Allocations are not allowed in the
   * callback function, you therefore cannot manipulate objects (set
   * or delete properties for example) since it is possible such
   * operations will result in the allocation of objects.
   */
  static void SetGlobalGCPrologueCallback(GCCallback);

  /**
   * Enables the host application to receive a notification after a
   * garbage collection.  Allocations are not allowed in the
   * callback function, you therefore cannot manipulate objects (set
   * or delete properties for example) since it is possible such
   * operations will result in the allocation of objects. It is possible
   * to specify the GCType filter for your callback. But it is not possible to
   * register the same callback function two times with different
   * GCType filters.
   */
  static void AddGCEpilogueCallback(
      GCEpilogueCallback callback, GCType gc_type_filter = kGCTypeAll);

  /**
   * This function removes callback which was installed by
   * AddGCEpilogueCallback function.
   */
  static void RemoveGCEpilogueCallback(GCEpilogueCallback callback);

  /**
   * The function is deprecated. Please use AddGCEpilogueCallback instead.
   * Enables the host application to receive a notification after a
   * major garbage collection.  Allocations are not allowed in the
   * callback function, you therefore cannot manipulate objects (set
   * or delete properties for example) since it is possible such
   * operations will result in the allocation of objects.
   */
  static void SetGlobalGCEpilogueCallback(GCCallback);

  /**
   * Enables the host application to provide a mechanism to be notified
   * and perform custom logging when V8 Allocates Executable Memory.
   */
  static void AddMemoryAllocationCallback(MemoryAllocationCallback callback,
                                          ObjectSpace space,
                                          AllocationAction action);

  /**
   * Removes callback that was installed by AddMemoryAllocationCallback.
   */
  static void RemoveMemoryAllocationCallback(MemoryAllocationCallback callback);

  /**
   * Adds a callback to notify the host application when a script finished
   * running.  If a script re-enters the runtime during executing, the
   * CallCompletedCallback is only invoked when the outer-most script
   * execution ends.  Executing scripts inside the callback do not trigger
   * further callbacks.
   */
  static void AddCallCompletedCallback(CallCompletedCallback callback);

  /**
   * Removes callback that was installed by AddCallCompletedCallback.
   */
  static void RemoveCallCompletedCallback(CallCompletedCallback callback);

  /**
   * Allows the host application to group objects together. If one
   * object in the group is alive, all objects in the group are alive.
   * After each garbage collection, object groups are removed. It is
   * intended to be used in the before-garbage-collection callback
   * function, for instance to simulate DOM tree connections among JS
   * wrapper objects. Object groups for all dependent handles need to
   * be provided for kGCTypeMarkSweepCompact collections, for all other
   * garbage collection types it is sufficient to provide object groups
   * for partially dependent handles only.
   * See v8-profiler.h for RetainedObjectInfo interface description.
   */
  static void AddObjectGroup(Persistent<Value>* objects,
                             size_t length,
                             RetainedObjectInfo* info = NULL);

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
   * Allows the host application to provide a callback which can be used
   * as a source of entropy for random number generators.
   */
  static void SetEntropySource(EntropySource source);

  /**
   * Allows the host application to provide a callback that allows v8 to
   * cooperate with a profiler that rewrites return addresses on stack.
   */
  static void SetReturnAddressLocationResolver(
      ReturnAddressLocationResolver return_address_resolver);

  /**
   * Allows the host application to provide the address of a function that's
   * invoked on entry to every V8-generated function.
   * Note that \p entry_hook is invoked at the very start of each
   * generated function.
   *
   * \param entry_hook a function that will be invoked on entry to every
   *   V8-generated function.
   * \returns true on success on supported platforms, false on failure.
   * \note Setting a new entry hook function when one is already active will
   *   fail.
   */
  static bool SetFunctionEntryHook(FunctionEntryHook entry_hook);

  /**
   * Allows the host application to provide the address of a function that is
   * notified each time code is added, moved or removed.
   *
   * \param options options for the JIT code event handler.
   * \param event_handler the JIT code event handler, which will be invoked
   *     each time code is added, moved or removed.
   * \note \p event_handler won't get notified of existent code.
   * \note since code removal notifications are not currently issued, the
   *     \p event_handler may get notifications of code that overlaps earlier
   *     code notifications. This happens when code areas are reused, and the
   *     earlier overlapping code areas should therefore be discarded.
   * \note the events passed to \p event_handler and the strings they point to
   *     are not guaranteed to live past each call. The \p event_handler must
   *     copy strings and other parameters it needs to keep around.
   * \note the set of events declared in JitCodeEvent::EventType is expected to
   *     grow over time, and the JitCodeEvent structure is expected to accrue
   *     new members. The \p event_handler function must ignore event codes
   *     it does not recognize to maintain future compatibility.
   */
  static void SetJitCodeEventHandler(JitCodeEventOptions options,
                                     JitCodeEventHandler event_handler);

  /**
   * Adjusts the amount of registered external memory.  Used to give
   * V8 an indication of the amount of externally allocated memory
   * that is kept alive by JavaScript objects.  V8 uses this to decide
   * when to perform global garbage collections.  Registering
   * externally allocated memory will trigger global garbage
   * collections more often than otherwise in an attempt to garbage
   * collect the JavaScript objects keeping the externally allocated
   * memory alive.
   *
   * \param change_in_bytes the change in externally allocated memory
   *   that is kept alive by JavaScript objects.
   * \returns the adjusted value.
   */
  static intptr_t AdjustAmountOfExternalAllocatedMemory(
      intptr_t change_in_bytes);

  /**
   * Suspends recording of tick samples in the profiler.
   * When the V8 profiling mode is enabled (usually via command line
   * switches) this function suspends recording of tick samples.
   * Profiling ticks are discarded until ResumeProfiler() is called.
   *
   * See also the --prof and --prof_auto command line switches to
   * enable V8 profiling.
   */
  static void PauseProfiler();

  /**
   * Resumes recording of tick samples in the profiler.
   * See also PauseProfiler().
   */
  static void ResumeProfiler();

  /**
   * Return whether profiler is currently paused.
   */
  static bool IsProfilerPaused();

  /**
   * Retrieve the V8 thread id of the calling thread.
   *
   * The thread id for a thread should only be retrieved after the V8
   * lock has been acquired with a Locker object with that thread.
   */
  static int GetCurrentThreadId();

  /**
   * Forcefully terminate execution of a JavaScript thread.  This can
   * be used to terminate long-running scripts.
   *
   * TerminateExecution should only be called when then V8 lock has
   * been acquired with a Locker object.  Therefore, in order to be
   * able to terminate long-running threads, preemption must be
   * enabled to allow the user of TerminateExecution to acquire the
   * lock.
   *
   * The termination is achieved by throwing an exception that is
   * uncatchable by JavaScript exception handlers.  Termination
   * exceptions act as if they were caught by a C++ TryCatch exception
   * handler.  If forceful termination is used, any C++ TryCatch
   * exception handler that catches an exception should check if that
   * exception is a termination exception and immediately return if
   * that is the case.  Returning immediately in that case will
   * continue the propagation of the termination exception if needed.
   *
   * The thread id passed to TerminateExecution must have been
   * obtained by calling GetCurrentThreadId on the thread in question.
   *
   * \param thread_id The thread id of the thread to terminate.
   */
  static void TerminateExecution(int thread_id);

  /**
   * Forcefully terminate the current thread of JavaScript execution
   * in the given isolate. If no isolate is provided, the default
   * isolate is used.
   *
   * This method can be used by any thread even if that thread has not
   * acquired the V8 lock with a Locker object.
   *
   * \param isolate The isolate in which to terminate the current JS execution.
   */
  static void TerminateExecution(Isolate* isolate = NULL);

  /**
   * Is V8 terminating JavaScript execution.
   *
   * Returns true if JavaScript execution is currently terminating
   * because of a call to TerminateExecution.  In that case there are
   * still JavaScript frames on the stack and the termination
   * exception is still active.
   *
   * \param isolate The isolate in which to check.
   */
  static bool IsExecutionTerminating(Isolate* isolate = NULL);

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
   * Get statistics about the heap memory usage.
   */
  static void GetHeapStatistics(HeapStatistics* heap_statistics);

  /**
   * Iterates through all external resources referenced from current isolate
   * heap.  GC is not invoked prior to iterating, therefore there is no
   * guarantee that visited objects are still alive.
   */
  static void VisitExternalResources(ExternalResourceVisitor* visitor);

  /**
   * Iterates through all the persistent handles in the current isolate's heap
   * that have class_ids.
   */
  static void VisitHandlesWithClassIds(PersistentHandleVisitor* visitor);

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

  /**
   * Optional notification that a context has been disposed. V8 uses
   * these notifications to guide the GC heuristic. Returns the number
   * of context disposals - including this one - since the last time
   * V8 had a chance to clean up.
   */
  static int ContextDisposedNotification();

 private:
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
   * For certain types of exceptions, it makes no sense to continue
   * execution.
   *
   * Currently, the only type of exception that can be caught by a
   * TryCatch handler and for which it does not make sense to continue
   * is termination exception.  Such exceptions are thrown when the
   * TerminateExecution methods are called to terminate a long-running
   * script.
   *
   * If CanContinue returns false, the correct action is to perform
   * any C++ cleanup needed and then return.
   */
  bool CanContinue() const;

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
   * Returns the .stack property of the thrown object.  If no .stack
   * property is present an empty handle is returned.
   */
  Local<Value> StackTrace() const;

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

  /**
   * Set verbosity of the external exception handler.
   *
   * By default, exceptions that are caught by an external exception
   * handler are not reported.  Call SetVerbose with true on an
   * external exception handler to have exceptions caught by the
   * handler reported as if they were not caught.
   */
  void SetVerbose(bool value);

  /**
   * Set whether or not this TryCatch should capture a Message object
   * which holds source information about where the exception
   * occurred.  True by default.
   */
  void SetCaptureMessage(bool value);

 private:
};


// --- Context ---


/**
 * Ignore
 */
class V8EXPORT ExtensionConfiguration {
 public:
  ExtensionConfiguration(int name_count, const char* names[])
      : name_count_(name_count), names_(names) { }
 private:
  friend class ImplementationUtilities;
  int name_count_;
  const char** names_;
};


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

  /**
   * Detaches the global object from its context before
   * the global object can be reused to create a new context.
   */
  void DetachGlobal();

  /**
   * Reattaches a global object to a context.  This can be used to
   * restore the connection between a global object and a context
   * after DetachGlobal has been called.
   *
   * \param global_object The global object to reattach to the
   *   context.  For this to work, the global object must be the global
   *   object that was associated with this context before a call to
   *   DetachGlobal.
   */
  void ReattachGlobal(Handle<Object> global_object);

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

  /** Returns the last entered context. */
  static Local<Context> GetEntered();

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
   * Sets the security token for the context.  To access an object in
   * another context, the security tokens must match.
   */
  void SetSecurityToken(Handle<Value> token);

  /** Restores the security token to the default value. */
  void UseDefaultSecurityToken();

  /** Returns the security token of this context.*/
  Handle<Value> GetSecurityToken();

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

  /** Returns true if the context has experienced an out of memory situation. */
  bool HasOutOfMemoryException();

  /** Returns true if V8 has a current context. */
  static bool InContext();

  /**
   * Associate an additional data object with the context. This is mainly used
   * with the debugger to provide additional information on the context through
   * the debugger API.
   */
  void SetData(Handle<Value> data);
  Local<Value> GetData();

  /**
   * Control whether code generation from strings is allowed. Calling
   * this method with false will disable 'eval' and the 'Function'
   * constructor for code running in this context. If 'eval' or the
   * 'Function' constructor are used an exception will be thrown.
   *
   * If code generation from strings is not allowed the
   * V8::AllowCodeGenerationFromStrings callback will be invoked if
   * set before blocking the call to 'eval' or the 'Function'
   * constructor. If that callback returns true, the call will be
   * allowed, otherwise an exception will be thrown. If no callback is
   * set an exception will be thrown.
   */
  void AllowCodeGenerationFromStrings(bool allow);

  /**
   * Returns true if code generation from strings is allowed for the context.
   * For more details see AllowCodeGenerationFromStrings(bool) documentation.
   */
  bool IsCodeGenerationFromStringsAllowed();

  /**
   * Sets the error description for the exception that is thrown when
   * code generation from strings is not allowed and 'eval' or the 'Function'
   * constructor are called.
   */
  void SetErrorMessageForCodeGenerationFromStrings(Handle<String> message);

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

/**
 * A struct for exporting HeapStats data from V8, using "push" model.
 */
struct HeapStatsUpdate;


/**
 * An interface for exporting data from V8, using "push" model.
 */
class V8EXPORT OutputStream {  // NOLINT
 public:
  enum OutputEncoding {
    kAscii = 0  // 7-bit ASCII.
  };
  enum WriteResult {
    kContinue = 0,
    kAbort = 1
  };
  virtual ~OutputStream() {}
  /** Notify about the end of stream. */
  virtual void EndOfStream() = 0;
  /** Get preferred output chunk size. Called only once. */
  virtual int GetChunkSize() { return 1024; }
  /** Get preferred output encoding. Called only once. */
  virtual OutputEncoding GetOutputEncoding() { return kAscii; }
  /**
   * Writes the next chunk of snapshot data into the stream. Writing
   * can be stopped by returning kAbort as function result. EndOfStream
   * will not be called in case writing was aborted.
   */
  virtual WriteResult WriteAsciiChunk(char* data, int size) = 0;
  /**
   * Writes the next chunk of heap stats data into the stream. Writing
   * can be stopped by returning kAbort as function result. EndOfStream
   * will not be called in case writing was aborted.
   */
  virtual WriteResult WriteHeapStatsChunk(HeapStatsUpdate* data, int count) {
    (void)data;
    (void)count;

    return kAbort;
  };
};

}  // namespace v8


#undef V8EXPORT
#undef TYPE_CHECK


#endif  // V8_H_
