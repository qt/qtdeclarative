/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qv4compilationunitmapper_p.h"

#include "qv4compileddata_p.h"
#include <QFileInfo>
#include <QDateTime>

QT_BEGIN_NAMESPACE

using namespace QV4;

CompilationUnitMapper::CompilationUnitMapper()
    : dataPtr(nullptr)
{

}

CompiledData::Unit *CompilationUnitMapper::open(const QString &sourcePath, QString *errorString)
{
    close();

    f.setFileName(sourcePath + QLatin1Char('c'));
    if (!f.open(QIODevice::ReadOnly)) {
        *errorString = f.errorString();
        return nullptr;
    }

    CompiledData::Unit header;
    qint64 bytesRead = f.read(reinterpret_cast<char *>(&header), sizeof(header));

    if (bytesRead != sizeof(header)) {
        *errorString = QStringLiteral("File too small for the header fields");
        return nullptr;
    }

    if (strncmp(header.magic, CompiledData::magic_str, sizeof(header.magic))) {
        *errorString = QStringLiteral("Magic bytes in the header do not match");
        return nullptr;
    }

    if (header.version != quint32(QV4_DATA_STRUCTURE_VERSION)) {
        *errorString = QString::fromUtf8("V4 data structure version mismatch. Found %1 expected %2").arg(header.version, 0, 16).arg(QV4_DATA_STRUCTURE_VERSION, 0, 16);
        return nullptr;
    }

    if (header.qtVersion != quint32(QT_VERSION)) {
        *errorString = QString::fromUtf8("Qt version mismatch. Found %1 expected %2").arg(header.qtVersion, 0, 16).arg(QT_VERSION, 0, 16);
        return nullptr;
    }

    {
        QFileInfo sourceCode(sourcePath);
        if (sourceCode.exists() && sourceCode.lastModified().toMSecsSinceEpoch() != header.sourceTimeStamp) {
            *errorString = QStringLiteral("QML source file has a different time stamp than cached file.");
            return nullptr;
        }
    }

    // Data structure and qt version matched, so now we can access the rest of the file safely.

    dataPtr = f.map(/*offset*/0, f.size());
    if (!dataPtr) {
        *errorString = f.errorString();
        return nullptr;
    }

    return reinterpret_cast<CompiledData::Unit*>(dataPtr);
}

void CompilationUnitMapper::close()
{
    f.close();
    dataPtr = nullptr;
}

QT_END_NAMESPACE
