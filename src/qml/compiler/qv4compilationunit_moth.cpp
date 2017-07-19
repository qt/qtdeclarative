/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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
#include "qv4compilationunit_moth_p.h"
#ifndef V4_BOOTSTRAP
#include <private/qv4function_p.h>
#include <private/qv4vme_moth_p.h>
#endif
#include <wtf/StdLibExtras.h>

using namespace QV4;
using namespace QV4::Moth;

CompilationUnit::~CompilationUnit()
{
}

#if !defined(V4_BOOTSTRAP)

void CompilationUnit::linkBackendToEngine(QV4::ExecutionEngine *engine)
{
    runtimeFunctions.resize(data->functionTableSize);
    runtimeFunctions.fill(0);
    for (int i = 0 ;i < runtimeFunctions.size(); ++i) {
        const QV4::CompiledData::Function *compiledFunction = data->functionAt(i);

        QV4::Function *runtimeFunction = new QV4::Function(engine, this, compiledFunction, &VME::exec);
        runtimeFunction->codeData = reinterpret_cast<const uchar *>(codeRefs.at(i).constData());
        runtimeFunctions[i] = runtimeFunction;
    }
}

bool CompilationUnit::memoryMapCode(QString *errorString)
{
    Q_UNUSED(errorString);
    codeRefs.resize(data->functionTableSize);

    const char *basePtr = reinterpret_cast<const char *>(data);

    for (uint i = 0; i < data->functionTableSize; ++i) {
        const CompiledData::Function *compiledFunction = data->functionAt(i);
        const char *codePtr = const_cast<const char *>(reinterpret_cast<const char *>(basePtr + compiledFunction->codeOffset));
#ifdef MOTH_THREADED_INTERPRETER
        // for the threaded interpreter we need to make a copy of the data because it needs to be
        // modified for the instruction handler addresses.
        QByteArray code(codePtr, compiledFunction->codeSize);
#else
        QByteArray code = QByteArray::fromRawData(codePtr, compiledFunction->codeSize);
#endif
        codeRefs[i] = code;
    }

    return true;
}

#endif // V4_BOOTSTRAP

void CompilationUnit::prepareCodeOffsetsForDiskStorage(CompiledData::Unit *unit)
{
    const int codeAlignment = 16;
    quint64 offset = WTF::roundUpToMultipleOf(codeAlignment, unit->unitSize);
    Q_ASSERT(int(unit->functionTableSize) == codeRefs.size());
    for (int i = 0; i < codeRefs.size(); ++i) {
        CompiledData::Function *compiledFunction = const_cast<CompiledData::Function *>(unit->functionAt(i));
        compiledFunction->codeOffset = offset;
        compiledFunction->codeSize = codeRefs.at(i).size();
        offset = WTF::roundUpToMultipleOf(codeAlignment, offset + compiledFunction->codeSize);
    }
}

bool CompilationUnit::saveCodeToDisk(QIODevice *device, const CompiledData::Unit *unit, QString *errorString)
{
    Q_ASSERT(device->pos() == unit->unitSize);
    Q_ASSERT(device->atEnd());
    Q_ASSERT(int(unit->functionTableSize) == codeRefs.size());

    QByteArray padding;

    for (int i = 0; i < codeRefs.size(); ++i) {
        const CompiledData::Function *compiledFunction = unit->functionAt(i);

        if (device->pos() > qint64(compiledFunction->codeOffset)) {
            *errorString = QStringLiteral("Invalid state of cache file to write.");
            return false;
        }

        const quint64 paddingSize = compiledFunction->codeOffset - device->pos();
        padding.fill(0, paddingSize);
        qint64 written = device->write(padding);
        if (written != padding.size()) {
            *errorString = device->errorString();
            return false;
        }

        QByteArray code = codeRefs.at(i);

        written = device->write(code.constData(), compiledFunction->codeSize);
        if (written != qint64(compiledFunction->codeSize)) {
            *errorString = device->errorString();
            return false;
        }
    }
    return true;
}
