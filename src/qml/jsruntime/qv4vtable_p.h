/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QV4VTABLE_P_H
#define QV4VTABLE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qv4global_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

struct VTable
{
    typedef void (*Destroy)(Heap::Base *);

    const VTable * const parent;
    quint32 inlinePropertyOffset : 16;
    quint32 nInlineProperties : 16;
    quint8 isExecutionContext;
    quint8 isString;
    quint8 isObject;
    quint8 isFunctionObject;
    quint8 isErrorObject;
    quint8 isArrayData;
    quint8 isStringOrSymbol;
    quint8 type;
    quint8 unused[4];
    const char *className;
    Destroy destroy;
    void (*markObjects)(Heap::Base *, MarkStack *markStack);
    bool (*isEqualTo)(Managed *m, Managed *other);
};

#define Q_VTABLE_FUNCTION(classname, func) \
    (classname::func == QV4::Managed::func ? 0 : classname::func)

// Q_VTABLE_FUNCTION triggers a bogus tautological-compare warning in GCC6+
#if (defined(Q_CC_GNU) && Q_CC_GNU >= 600)
#define QT_WARNING_SUPPRESS_GCC_TAUTOLOGICAL_COMPARE_ON \
    QT_WARNING_PUSH; \
    QT_WARNING_DISABLE_GCC("-Wtautological-compare")

#define QT_WARNING_SUPPRESS_GCC_TAUTOLOGICAL_COMPARE_OFF \
    ;QT_WARNING_POP
#elif defined(Q_CC_CLANG) && Q_CC_CLANG >= 306
#define QT_WARNING_SUPPRESS_GCC_TAUTOLOGICAL_COMPARE_ON \
    QT_WARNING_PUSH; \
    QT_WARNING_DISABLE_CLANG("-Wtautological-compare")

#define QT_WARNING_SUPPRESS_GCC_TAUTOLOGICAL_COMPARE_OFF \
    ;QT_WARNING_POP
#else
#define QT_WARNING_SUPPRESS_GCC_TAUTOLOGICAL_COMPARE_ON
#define QT_WARNING_SUPPRESS_GCC_TAUTOLOGICAL_COMPARE_OFF
#endif

#define DEFINE_MANAGED_VTABLE_INT(classname, parentVTable) \
{     \
    parentVTable, \
    (sizeof(classname::Data) + sizeof(QV4::Value) - 1)/sizeof(QV4::Value), \
    (sizeof(classname::Data) + (classname::NInlineProperties*sizeof(QV4::Value)) + QV4::Chunk::SlotSize - 1)/QV4::Chunk::SlotSize*QV4::Chunk::SlotSize/sizeof(QV4::Value) \
        - (sizeof(classname::Data) + sizeof(QV4::Value) - 1)/sizeof(QV4::Value), \
    classname::IsExecutionContext,   \
    classname::IsString,   \
    classname::IsObject,   \
    classname::IsFunctionObject,   \
    classname::IsErrorObject,   \
    classname::IsArrayData,   \
    classname::IsStringOrSymbol,   \
    classname::MyType,                          \
    { 0, 0, 0, 0 },                                          \
    #classname, \
    Q_VTABLE_FUNCTION(classname, destroy),                                    \
    classname::Data::markObjects,                                    \
    isEqualTo                                  \
} \

#define DEFINE_MANAGED_VTABLE(classname) \
QT_WARNING_SUPPRESS_GCC_TAUTOLOGICAL_COMPARE_ON \
const QV4::VTable classname::static_vtbl = DEFINE_MANAGED_VTABLE_INT(classname, 0) \
QT_WARNING_SUPPRESS_GCC_TAUTOLOGICAL_COMPARE_OFF

#define V4_OBJECT2(DataClass, superClass) \
    private: \
        DataClass() Q_DECL_EQ_DELETE; \
        Q_DISABLE_COPY(DataClass) \
    public: \
        Q_MANAGED_CHECK \
        typedef QV4::Heap::DataClass Data; \
        typedef superClass SuperClass; \
        static const QV4::ObjectVTable static_vtbl; \
        static inline const QV4::VTable *staticVTable() { return &static_vtbl.vTable; } \
        V4_MANAGED_SIZE_TEST \
        QV4::Heap::DataClass *d_unchecked() const { return static_cast<QV4::Heap::DataClass *>(m()); } \
        QV4::Heap::DataClass *d() const { \
            QV4::Heap::DataClass *dptr = d_unchecked(); \
            dptr->_checkIsInitialized(); \
            return dptr; \
        } \
        Q_STATIC_ASSERT(std::is_trivial< QV4::Heap::DataClass >::value);

#define V4_PROTOTYPE(p) \
    static QV4::Object *defaultPrototype(QV4::ExecutionEngine *e) \
    { return e->p(); }

typedef ReturnedValue (*jsCallFunction)(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
typedef ReturnedValue (*jsConstructFunction)(const FunctionObject *, const Value *argv, int argc);

struct ObjectVTable
{
    VTable vTable;
    jsCallFunction call;
    jsConstructFunction callAsConstructor;
    ReturnedValue (*get)(const Managed *, PropertyKey id, const Value *receiver, bool *hasProperty);
    bool (*put)(Managed *, PropertyKey id, const Value &value, Value *receiver);
    bool (*deleteProperty)(Managed *m, PropertyKey id);
    bool (*hasProperty)(const Managed *m, PropertyKey id);
    PropertyAttributes (*getOwnProperty)(Managed *m, PropertyKey id, Property *p);
    bool (*defineOwnProperty)(Managed *m, PropertyKey id, const Property *p, PropertyAttributes attrs);
    bool (*isExtensible)(const Managed *);
    bool (*preventExtensions)(Managed *);
    Heap::Object *(*getPrototypeOf)(const Managed *);
    bool (*setPrototypeOf)(Managed *, const Object *);
    qint64 (*getLength)(const Managed *m);
    void (*advanceIterator)(Managed *m, ObjectIterator *it, Value *name, uint *index, Property *p, PropertyAttributes *attributes);
    ReturnedValue (*instanceOf)(const Object *typeObject, const Value &var);
};

#define DEFINE_OBJECT_VTABLE_BASE(classname) \
const QV4::ObjectVTable classname::static_vtbl =    \
{     \
    DEFINE_MANAGED_VTABLE_INT(classname, (std::is_same<classname::SuperClass, Object>::value) ? nullptr : &classname::SuperClass::static_vtbl.vTable), \
    call,                                       \
    callAsConstructor,                          \
    get,                                        \
    put,                                        \
    deleteProperty,                             \
    hasProperty,                                \
    getOwnProperty,                             \
    defineOwnProperty,                          \
    isExtensible,                               \
    preventExtensions,                          \
    getPrototypeOf,                             \
    setPrototypeOf,                             \
    getLength,                                  \
    advanceIterator,                            \
    instanceOf                                  \
}

#define DEFINE_OBJECT_VTABLE(classname) \
QT_WARNING_SUPPRESS_GCC_TAUTOLOGICAL_COMPARE_ON \
DEFINE_OBJECT_VTABLE_BASE(classname) \
QT_WARNING_SUPPRESS_GCC_TAUTOLOGICAL_COMPARE_OFF

#define DEFINE_OBJECT_TEMPLATE_VTABLE(classname) \
QT_WARNING_SUPPRESS_GCC_TAUTOLOGICAL_COMPARE_ON \
template<> DEFINE_OBJECT_VTABLE_BASE(classname) \
QT_WARNING_SUPPRESS_GCC_TAUTOLOGICAL_COMPARE_OFF


}

QT_END_NAMESPACE

#endif
