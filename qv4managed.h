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
#ifndef QMLJS_MANAGED_H
#define QMLJS_MANAGED_H

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QDebug>
#include <wtf/Platform.h>

namespace QQmlJS {

namespace VM {

class MemoryManager;
struct Object;
struct ObjectPrototype;
struct ExecutionContext;
struct ScriptFunction;

struct Managed
{
private:
    void *operator new(size_t);
    Managed(const Managed &other);
    void operator = (const Managed &other);

protected:
    Managed() : markBit(0), inUse(1), extensible(true), isArray(false), isArgumentsObject(false), isString(false), isBuiltinFunction(false), unused(0) { }
    virtual ~Managed();

public:
    void *operator new(size_t size, MemoryManager *mm);
    void operator delete(void *ptr);

protected:
    virtual void getCollectables(QVector<Object *> &objects) = 0;

    union {
        Managed *nextFree;
        struct {
            quintptr markBit :  1;
            quintptr inUse   :  1;
            quintptr extensible : 1; // used by Object
            quintptr isArray : 1; // used by Object & Array
            quintptr isArgumentsObject : 1;
            quintptr isString : 1; // used by Object & StringObject
            quintptr isBuiltinFunction : 1; // used by FunctionObject
            quintptr needsActivation : 1; // used by FunctionObject
            quintptr usesArgumentsObject : 1; // used by FunctionObject
            quintptr strictMode : 1; // used by FunctionObject
#if CPU(X86_64)
            quintptr unused  : 55;
#elif CPU(X86)
            quintptr unused  : 23;
#else
#error "implement me"
#endif
        };
    };

private:
    friend class MemoryManager;
    friend struct Object;
    friend struct ObjectPrototype;
    friend class Array;
    friend struct ArrayPrototype;
    friend struct FunctionObject;
    friend struct ExecutionContext;
    friend struct ScriptFunction;
};

}
}

#endif
