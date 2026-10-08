// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qv4compilationunitmapper_p.h"

#include <private/qv4compileddata_p.h>

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

    void remove(const QString &file)
    {
        s_staticUnits.remove(file);
    }

private:
    QMutexLocker<QMutex> m_lock;

    static QMutex s_mutex;

    // We can copy the mappers around because they're all static.
    // We never unmap the files.
    static QHash<QString, CompilationUnitMapper> s_staticUnits;
};

QHash<QString, CompilationUnitMapper> StaticUnitCache::s_staticUnits;
QMutex StaticUnitCache::s_mutex;

CompiledData::Unit *CompilationUnitMapper::get(
        const QString &cacheFilePath, const QDateTime &sourceTimeStamp, QString *errorString)
{
    StaticUnitCache cache;

    CompilationUnitMapper mapper = cache.get(cacheFilePath);
    if (mapper.dataPtr) {
        auto *unit = reinterpret_cast<CompiledData::Unit *>(mapper.dataPtr);
        if (unit->verifyHeader(sourceTimeStamp, errorString)) {
            *this = mapper;
            return unit;
        }

        return nullptr;
    }

    CompiledData::Unit *data = open(cacheFilePath, sourceTimeStamp, errorString);
    if (data && (data->flags & CompiledData::Unit::StaticData)) {
        cache.set(cacheFilePath, *this);
        return data;
    } else {
        close();
        return nullptr;
    }
}

void CompilationUnitMapper::invalidate(const QString &cacheFilePath)
{
    StaticUnitCache cache;
    cache.remove(cacheFilePath);
}

QT_END_NAMESPACE
