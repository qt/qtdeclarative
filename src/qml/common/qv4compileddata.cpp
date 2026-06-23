// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical data-parser

#include "qv4compileddata_p.h"

#include <private/inlinecomponentutils_p.h>
#include <private/qqmlscriptdata_p.h>
#include <private/qqmltypenamecache_p.h>
#include <private/qv4resolvedtypereference_p.h>

#include <QtQml/qqmlfile.h>

#include <QtCore/qdir.h>
#include <QtCore/qscopeguard.h>
#include <QtCore/qstandardpaths.h>
#include <QtCore/qxpfunctional.h>

QT_BEGIN_NAMESPACE

namespace QV4 {
namespace CompiledData {


bool Unit::verifyHeader(QDateTime expectedSourceTimeStamp, QString *errorString) const
{
    if (strncmp(magic, CompiledData::magic_str, sizeof(magic))) {
        *errorString = QStringLiteral("Magic bytes in the header do not match");
        return false;
    }

    if (version != quint32(QV4_DATA_STRUCTURE_VERSION)) {
        *errorString = QString::fromUtf8("V4 data structure version mismatch. Found %1 expected %2")
                               .arg(quint32(version), 0, 16).arg(QV4_DATA_STRUCTURE_VERSION, 0, 16);
        return false;
    }

    switch (sourceTimeStamp) {
    case 0:
        // No validation necessary
        return true;
    case -1:
        // Content-hash mode: the unit is validated against sourceChecksum rather than the source
        // file's time stamp. The actual comparison is done in CompilationUnit::loadFromDisk, which
        // has access to the source code.
        return true;
    default:
        break;
    }

    // Files from the resource system do not have any time stamps, so fall back to the application
    // executable.
    if (!expectedSourceTimeStamp.isValid()) {
        expectedSourceTimeStamp = QFileInfo(QCoreApplication::applicationFilePath()).lastModified();
        if (!expectedSourceTimeStamp.isValid()) {
            *errorString =
                    QStringLiteral("Failed to get valid timestamp from application executable");
            return false;
        }
    }
    if (expectedSourceTimeStamp.toMSecsSinceEpoch() != sourceTimeStamp) {
        *errorString =
                QStringLiteral("QML source file has a different time stamp than cached file.");
        return false;
    }

    return true;
}

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

CompilationUnit::CompilationUnit(
        const Unit *unitData, const QString &fileName, const QString &finalUrlString)
{
    setUnitData(unitData, nullptr, fileName, finalUrlString);
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
        const QUrl &url, const QDateTime &sourceTimeStamp,
        qxp::function_ref<QByteArray() const> sourceChecksum, QString *errorString)
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

        if (mappedUnit->sourceTimeStamp == -1) {
            const QByteArray checksum = sourceChecksum();
            if (checksum.size() != sizeof(mappedUnit->sourceChecksum)
                || memcmp(mappedUnit->sourceChecksum, checksum.constData(), checksum.size()) != 0) {
                *errorString = QStringLiteral(
                        "QML source file has a different content checksum than cached file.");
                continue;
            }
        }

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

bool CompilationUnit::saveToDisk(
        const QUrl &unitUrl, qxp::function_ref<QByteArray() const> sourceChecksum,
        QString *errorString) const
{
    if (!QQmlFile::isLocalFile(unitUrl)) {
        *errorString = QStringLiteral("File has to be a local file.");
        return false;
    }

    Unit *mutableUnit = const_cast<Unit *>(unitData());
    const QByteArray checksum = sourceChecksum();
    if (checksum.size() != sizeof(mutableUnit->sourceChecksum)) {
        *errorString = QStringLiteral("Failed to compute source code checksum");
        return false;
    }

    // Switch the unit to content-hash mode for the duration of the write. sourceTimeStamp and
    // sourceChecksum live before md5Checksum and are therefore not covered by it, so we can patch
    // them without recomputing the integrity checksum. Restore them afterwards because the
    // in-memory unit is kept around and may still be used with its original time stamp.
    const qint64 oldTimeStamp = mutableUnit->sourceTimeStamp;
    char oldChecksum[sizeof(mutableUnit->sourceChecksum)];
    memcpy(oldChecksum, mutableUnit->sourceChecksum, sizeof(oldChecksum));
    mutableUnit->sourceTimeStamp = -1;
    memcpy(mutableUnit->sourceChecksum, checksum.constData(), sizeof(mutableUnit->sourceChecksum));
    const auto restore = qScopeGuard([&]() {
        mutableUnit->sourceTimeStamp = oldTimeStamp;
        memcpy(mutableUnit->sourceChecksum, oldChecksum, sizeof(oldChecksum));
    });

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

void CompiledData::CompilationUnit::finalizeCompositeType(const QQmlType &type)
{
    // Add to type registry of composites
    if (propertyCaches.needsVMEMetaObject(/*root object*/0)) {
        // qmlType is only valid for types that have references to themselves.
        if (type.isValid()) {
            qmlType = type;
        } else {
            qmlType = QQmlMetaType::findCompositeType(
                    url(), this, (unitData()->flags & CompiledData::Unit::IsSingleton)
                            ? QQmlMetaType::Singleton
                            : QQmlMetaType::NonSingleton);
        }

        QQmlMetaType::registerInternalCompositeType(this);
    } else {
        const QV4::CompiledData::Object *obj = objectAt(/*root object*/0);
        auto *typeRef = resolvedTypes.value(obj->inheritedTypeNameIndex);
        Q_ASSERT(typeRef);
        qmlType = typeRef->type();
    }
}

bool CompilationUnit::verifyChecksum(const DependentTypesHasher &dependencyHasher) const
{
    if (!dependencyHasher) {
        for (size_t i = 0; i < sizeof(data->dependencyMD5Checksum); ++i) {
            if (data->dependencyMD5Checksum[i] != 0)
                return false;
        }
        return true;
    }
    const QByteArray checksum = dependencyHasher();
    return checksum.size() == sizeof(data->dependencyMD5Checksum)
            && memcmp(data->dependencyMD5Checksum, checksum.constData(),
                      sizeof(data->dependencyMD5Checksum)) == 0;
}

QQmlType CompilationUnit::qmlTypeForComponent(const QString &inlineComponentName) const
{
    if (inlineComponentName.isEmpty())
        return qmlType;
    return inlineComponentData[inlineComponentName].qmlType;
}

} // namespace CompiledData
} // namespace QV4

QT_END_NAMESPACE
