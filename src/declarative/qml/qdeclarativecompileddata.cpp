/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "private/qdeclarativecompiler_p.h"
#include "qdeclarativeengine.h"
#include "qdeclarativecomponent.h"
#include "private/qdeclarativecomponent_p.h"
#include "qdeclarativecontext.h"
#include "private/qdeclarativecontext_p.h"
#ifdef QML_THREADED_VME_INTERPRETER
#include "private/qdeclarativevme_p.h"
#endif

#include <QtCore/qdebug.h>

#include <private/qobject_p.h>

QT_BEGIN_NAMESPACE

int QDeclarativeCompiledData::pack(const char *data, size_t size)
{
    const char *p = packData.constData();
    unsigned int ps = packData.size();

    for (unsigned int ii = 0; (ii + size) <= ps; ii += sizeof(int)) {
        if (0 == ::memcmp(p + ii, data, size))
            return ii;
    }

    int rv = packData.size();
    packData.append(data, size);
    return rv;
}

int QDeclarativeCompiledData::indexForString(const QString &data)
{
    int idx = primitives.indexOf(data);
    if (idx == -1) {
        idx = primitives.count();
        primitives << data;
    }
    return idx;
}

int QDeclarativeCompiledData::indexForByteArray(const QByteArray &data)
{
    int idx = datas.indexOf(data);
    if (idx == -1) {
        idx = datas.count();
        datas << data;
    }
    return idx;
}

int QDeclarativeCompiledData::indexForUrl(const QUrl &data)
{
    int idx = urls.indexOf(data);
    if (idx == -1) {
        idx = urls.count();
        urls << data;
    }
    return idx;
}

QDeclarativeCompiledData::QDeclarativeCompiledData(QDeclarativeEngine *engine)
: engine(engine), importCache(0), root(0), rootPropertyCache(0)
{
    Q_ASSERT(engine);

    bytecode.reserve(1024);
}

void QDeclarativeCompiledData::destroy()
{
    if (engine && hasEngine())
        QDeclarativeEnginePrivate::deleteInEngineThread(engine, this);
    else
        delete this;
}

QDeclarativeCompiledData::~QDeclarativeCompiledData()
{
    clear();

    for (int ii = 0; ii < types.count(); ++ii) {
        if (types.at(ii).component)
            types.at(ii).component->release();
        if (types.at(ii).typePropertyCache)
            types.at(ii).typePropertyCache->release();
    }

    for (int ii = 0; ii < propertyCaches.count(); ++ii) 
        propertyCaches.at(ii)->release();

    for (int ii = 0; ii < contextCaches.count(); ++ii)
        contextCaches.at(ii)->release();

    for (int ii = 0; ii < scripts.count(); ++ii)
        scripts.at(ii)->release();

    if (importCache)
        importCache->release();

    if (rootPropertyCache)
        rootPropertyCache->release();
}

void QDeclarativeCompiledData::clear()
{
    for (int ii = 0; ii < v8bindings.count(); ++ii)
        qPersistentDispose(v8bindings[ii]);
    v8bindings.clear();
}

const QMetaObject *QDeclarativeCompiledData::TypeReference::metaObject() const
{
    if (type) {
        return type->metaObject();
    } else {
        Q_ASSERT(component);
        return component->root;
    }
}

/*!
Returns the property cache, if one alread exists.  The cache is not referenced.
*/
QDeclarativePropertyCache *QDeclarativeCompiledData::TypeReference::propertyCache() const
{
    if (type)
        return typePropertyCache;
    else
        return component->rootPropertyCache;
}

/*!
Returns the property cache, creating one if it doesn't already exist.  The cache is not referenced.
*/
QDeclarativePropertyCache *QDeclarativeCompiledData::TypeReference::createPropertyCache(QDeclarativeEngine *engine) 
{
    if (typePropertyCache) {
        return typePropertyCache;
    } else if (type) {
        typePropertyCache = QDeclarativeEnginePrivate::get(engine)->cache(type->metaObject());
        typePropertyCache->addref();
        return typePropertyCache;
    } else {
        return component->rootPropertyCache;
    }
}


void QDeclarativeCompiledData::dumpInstructions()
{
    if (!name.isEmpty())
        qWarning() << name;
    qWarning().nospace() << "Index\tOperation\t\tData1\tData2\tData3\tComments";
    qWarning().nospace() << "-------------------------------------------------------------------------------";

    const char *instructionStream = bytecode.constData();
    const char *endInstructionStream = bytecode.constData() + bytecode.size();

    int instructionCount = 0;
    while (instructionStream < endInstructionStream) {
        QDeclarativeInstruction *instr = (QDeclarativeInstruction *)instructionStream;
        dump(instr, instructionCount);
        instructionStream += QDeclarativeInstruction::size(instructionType(instr));
        instructionCount++;
    }

    qWarning().nospace() << "-------------------------------------------------------------------------------";
}

int QDeclarativeCompiledData::addInstructionHelper(QDeclarativeInstruction::Type type, QDeclarativeInstruction &instr)
{
#ifdef QML_THREADED_VME_INTERPRETER
    instr.common.code = QDeclarativeVME::instructionJumpTable()[static_cast<int>(type)];
#else
    instr.common.instructionType = type;
#endif
    int ptrOffset = bytecode.size();
    int size = QDeclarativeInstruction::size(type);
    if (bytecode.capacity() <= bytecode.size() + size)
        bytecode.reserve(bytecode.size() + size + 512);
    bytecode.append(reinterpret_cast<const char *>(&instr), size);
    return ptrOffset;
}

int QDeclarativeCompiledData::nextInstructionIndex() 
{ 
    return bytecode.size();
}

QDeclarativeInstruction *QDeclarativeCompiledData::instruction(int index) 
{ 
    return (QDeclarativeInstruction *)(bytecode.constData() + index);
}

QDeclarativeInstruction::Type QDeclarativeCompiledData::instructionType(const QDeclarativeInstruction *instr)
{
#ifdef QML_THREADED_VME_INTERPRETER
    void **jumpTable = QDeclarativeVME::instructionJumpTable();
    void *code = instr->common.code;

#  define QML_CHECK_INSTR_CODE(I, FMT) \
    if (jumpTable[static_cast<int>(QDeclarativeInstruction::I)] == code) \
        return QDeclarativeInstruction::I;

    FOR_EACH_QML_INSTR(QML_CHECK_INSTR_CODE)
    Q_ASSERT_X(false, Q_FUNC_INFO, "Invalid instruction address");
    return static_cast<QDeclarativeInstruction::Type>(0);
#  undef QML_CHECK_INSTR_CODE
#else
    return static_cast<QDeclarativeInstruction::Type>(instr->common.instructionType);
#endif
}

void QDeclarativeCompiledData::initialize(QDeclarativeEngine *engine)
{
    Q_ASSERT(!hasEngine());
    QDeclarativeCleanup::addToEngine(engine);
}

QT_END_NAMESPACE
