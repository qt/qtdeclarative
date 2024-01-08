// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4compileddata_p.h"

#include <private/qv4resolvedtypereference_p.h>

#include <QtQml/qqmlfile.h>

#include <QtCore/qdir.h>
#include <QtCore/qscopeguard.h>
#include <QtCore/qstandardpaths.h>

QT_BEGIN_NAMESPACE

namespace QV4 {
namespace CompiledData {

/*!
    \internal
    This function creates a temporary key vector and sorts it to guarantuee a stable
    hash. This is used to calculate a check-sum on dependent meta-objects.
 */
bool ResolvedTypeReferenceMap::addToHash(
        QCryptographicHash *hash, QHash<quintptr, QByteArray> *checksums) const
{
    std::vector<int> keys (size());
    int i = 0;
    for (auto it = constBegin(), end = constEnd(); it != end; ++it) {
        keys[i] = it.key();
        ++i;
    }
    std::sort(keys.begin(), keys.end());
    for (int key: keys) {
        if (!this->operator[](key)->addToHash(hash, checksums))
            return false;
    }

    return true;
}

CompilationUnit::~CompilationUnit()
{
    qDeleteAll(resolvedTypes);

    if (data) {
        if (data->qmlUnit() != qmlData)
            free(const_cast<QmlUnit *>(qmlData));
        qmlData = nullptr;

        if (!(data->flags & QV4::CompiledData::Unit::StaticData))
            free(const_cast<Unit *>(data));
    }
    data = nullptr;
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    delete [] constants;
    constants = nullptr;
#endif
}

QString CompilationUnit::localCacheFilePath(const QUrl &url)
{
    static const QByteArray envCachePath = qgetenv("QML_DISK_CACHE_PATH");

    const QString localSourcePath = QQmlFile::urlToLocalFileOrQrc(url);
    const QString cacheFileSuffix
            = QFileInfo(localSourcePath + QLatin1Char('c')).completeSuffix();
    QCryptographicHash fileNameHash(QCryptographicHash::Sha1);
    fileNameHash.addData(localSourcePath.toUtf8());
    QString directory = envCachePath.isEmpty()
            ? QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                    + QLatin1String("/qmlcache/")
            : QString::fromLocal8Bit(envCachePath) + QLatin1String("/");
    QDir::root().mkpath(directory);
    return directory + QString::fromUtf8(fileNameHash.result().toHex())
            + QLatin1Char('.') + cacheFileSuffix;
}

bool CompilationUnit::loadFromDisk(
        const QUrl &url, const QDateTime &sourceTimeStamp, QString *errorString)
{
    if (!QQmlFile::isLocalFile(url)) {
        *errorString = QStringLiteral("File has to be a local file.");
        return false;
    }

    const QString sourcePath = QQmlFile::urlToLocalFileOrQrc(url);
    auto cacheFile = std::make_unique<CompilationUnitMapper>();

    const QStringList cachePaths = { sourcePath + QLatin1Char('c'), localCacheFilePath(url) };
    for (const QString &cachePath : cachePaths) {
        Unit *mappedUnit = cacheFile->get(cachePath, sourceTimeStamp, errorString);
        if (!mappedUnit)
            continue;

        const Unit *oldData = unitData();
        const Unit * const oldDataPtr
                = (oldData && !(oldData->flags & Unit::StaticData))
                ? oldData
                : nullptr;

        auto dataPtrRevert = qScopeGuard([this, oldData](){
            setUnitData(oldData);
        });
        setUnitData(mappedUnit);

        if (mappedUnit->sourceFileIndex != 0) {
            if (mappedUnit->sourceFileIndex >=
                mappedUnit->stringTableSize + dynamicStrings.size()) {
                *errorString = QStringLiteral("QML source file index is invalid.");
                continue;
            }
            if (sourcePath !=
                QQmlFile::urlToLocalFileOrQrc(stringAt(mappedUnit->sourceFileIndex))) {
                *errorString = QStringLiteral("QML source file has moved to a different location.");
                continue;
            }
        }

        dataPtrRevert.dismiss();
        free(const_cast<Unit*>(oldDataPtr));
        backingFile = std::move(cacheFile);
        return true;
    }

    return false;
}

bool CompilationUnit::saveToDisk(const QUrl &unitUrl, QString *errorString)
{
    if (unitData()->sourceTimeStamp == 0) {
        *errorString = QStringLiteral("Missing time stamp for source file");
        return false;
    }

    if (!QQmlFile::isLocalFile(unitUrl)) {
        *errorString = QStringLiteral("File has to be a local file.");
        return false;
    }

    return SaveableUnitPointer(unitData()).saveToDisk<char>(
            [&unitUrl, errorString](const char *data, quint32 size) {
                const QString cachePath = localCacheFilePath(unitUrl);
                if (SaveableUnitPointer::writeDataToFile(
                            cachePath, data, size, errorString)) {
                    CompilationUnitMapper::invalidate(cachePath);
                    return true;
                }

                return false;
            });
}

QStringList CompilationUnit::moduleRequests() const
{
    QStringList requests;
    requests.reserve(data->moduleRequestTableSize);
    for (uint i = 0; i < data->moduleRequestTableSize; ++i)
        requests << stringAt(data->moduleRequestTable()[i]);
    return requests;
}

ResolvedTypeReference *CompilationUnit::resolvedType(QMetaType type) const
{
    for (ResolvedTypeReference *ref : std::as_const(resolvedTypes)) {
        if (ref->type().typeId() == type)
            return ref;
    }
    return nullptr;

}

int CompilationUnit::totalBindingsCount() const
{
    if (!icRootName)
        return m_totalBindingsCount;
    return inlineComponentData[*icRootName].totalBindingCount;
}

int CompilationUnit::totalObjectCount() const
{
    if (!icRootName)
        return m_totalObjectCount;
    return inlineComponentData[*icRootName].totalObjectCount;
}

int CompilationUnit::totalParserStatusCount() const
{
    if (!icRootName)
        return m_totalParserStatusCount;
    return inlineComponentData[*icRootName].totalParserStatusCount;
}

} // namespace CompiledData
} // namespace QV4

QT_END_NAMESPACE
