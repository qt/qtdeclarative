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

#include <private/qv4compileddata_p.h>
#include <private/qv4executablecompilationunit_p.h>

#include <QtCore/qdatetime.h>
#include <QtCore/qmutex.h>
#include <QtCore/qhash.h>

QT_BEGIN_NAMESPACE

using namespace QV4;

class StaticUnitCache
{
public:
    StaticUnitCache() : m_lock(&s_mutex) {}

    CompilationUnitMapper get(const QString &file)
    {
        const auto it = s_staticUnits.constFind(file);
        return it == s_staticUnits.constEnd() ? CompilationUnitMapper() : *it;
    }

    void set(const QString &file, const CompilationUnitMapper &staticUnit) {
        s_staticUnits.insert(file, staticUnit);
    }

private:
    QMutexLocker m_lock;

    static QMutex s_mutex;

    // We can copy the mappers around because they're all static, that is the dtors are noops.
    static QHash<QString, CompilationUnitMapper> s_staticUnits;
};

QHash<QString, CompilationUnitMapper> StaticUnitCache::s_staticUnits;
QMutex StaticUnitCache::s_mutex;

CompilationUnitMapper::~CompilationUnitMapper()
{
    close();
}

CompiledData::Unit *CompilationUnitMapper::get(
        const QString &cacheFilePath, const QDateTime &sourceTimeStamp, QString *errorString)
{
    StaticUnitCache cache;

    CompilationUnitMapper mapper = cache.get(cacheFilePath);
    if (mapper.dataPtr) {
        auto *unit = reinterpret_cast<CompiledData::Unit *>(mapper.dataPtr);
        if (ExecutableCompilationUnit::verifyHeader(unit, sourceTimeStamp, errorString)) {
            *this = mapper;
            return unit;
        }

        return nullptr;
    }

    CompiledData::Unit *data = open(cacheFilePath, sourceTimeStamp, errorString);
    if (data && (data->flags & CompiledData::Unit::StaticData))
        cache.set(cacheFilePath, *this);

    return data;
}

QT_END_NAMESPACE
