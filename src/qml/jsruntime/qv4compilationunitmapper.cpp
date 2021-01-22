/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
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
