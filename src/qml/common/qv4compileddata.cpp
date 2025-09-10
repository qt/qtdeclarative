// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4compileddata_p.h"

#include <private/inlinecomponentutils_p.h>
#include <private/qml_compile_hash_p.h>
#include <private/qqmlscriptdata_p.h>
#include <private/qqmltypenamecache_p.h>
#include <private/qv4resolvedtypereference_p.h>

#include <QtQml/qqmlfile.h>

#include <QtCore/qdir.h>
#include <QtCore/qscopeguard.h>
#include <QtCore/qstandardpaths.h>
#include <QtCore/qxpfunctional.h>

static_assert(QV4::CompiledData::QmlCompileHashSpace > QML_COMPILE_HASH_LENGTH);

#if defined(QML_COMPILE_HASH) && defined(QML_COMPILE_HASH_LENGTH) && QML_COMPILE_HASH_LENGTH > 0
#  ifdef Q_OS_LINUX
// Place on a separate section on Linux so it's easier to check from outside
// what the hash version is.
__attribute__((section(".qml_compile_hash")))
#  endif
const char qml_compile_hash[QV4::CompiledData::QmlCompileHashSpace] = QML_COMPILE_HASH;
static_assert(sizeof(QV4::CompiledData::Unit::libraryVersionHash) > QML_COMPILE_HASH_LENGTH,
              "Compile hash length exceeds reserved size in data structure. Please adjust and bump the format version");
#else
#  error "QML_COMPILE_HASH must be defined for the build of QtDeclarative to ensure version checking for cache files"
#endif

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
                               .arg(version, 0, 16).arg(QV4_DATA_STRUCTURE_VERSION, 0, 16);
        return false;
    }

    if (qtVersion != quint32(QT_VERSION)) {
        *errorString = QString::fromUtf8("Qt version mismatch. Found %1 expected %2")
                               .arg(qtVersion, 0, 16).arg(QT_VERSION, 0, 16);
        return false;
    }

    if (sourceTimeStamp) {
        // Files from the resource system do not have any time stamps, so fall back to the application
        // executable.
        if (!expectedSourceTimeStamp.isValid())
            expectedSourceTimeStamp = QFileInfo(QCoreApplication::applicationFilePath()).lastModified();

        if (expectedSourceTimeStamp.isValid()
            && expectedSourceTimeStamp.toMSecsSinceEpoch() != sourceTimeStamp) {
            *errorString = QStringLiteral("QML source file has a different time stamp than cached file.");
            return false;
        }
    }

#if defined(QML_COMPILE_HASH) && defined(QML_COMPILE_HASH_LENGTH) && QML_COMPILE_HASH_LENGTH > 0
    if (qstrncmp(qml_compile_hash, libraryVersionHash, QML_COMPILE_HASH_LENGTH) != 0) {
        *errorString = QStringLiteral("QML compile hashes don't match. Found %1 expected %2")
                               .arg(QString::fromLatin1(
                                            QByteArray(libraryVersionHash, QML_COMPILE_HASH_LENGTH)
                                                    .toPercentEncoding()),
                                    QString::fromLatin1(
                                            QByteArray(qml_compile_hash, QML_COMPILE_HASH_LENGTH)
                                                    .toPercentEncoding()));
        return false;
    }
#else
#error "QML_COMPILE_HASH must be defined for the build of QtDeclarative to ensure version checking for cache files"
#endif
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

int CompilationUnit::totalBindingsCount(const QString &inlineComponentRootName) const
{
    if (inlineComponentRootName.isEmpty())
        return m_totalBindingsCount;
    return inlineComponentData[inlineComponentRootName].totalBindingCount;
}

int CompilationUnit::totalObjectCount(const QString &inlineComponentRootName) const
{
    if (inlineComponentRootName.isEmpty())
        return m_totalObjectCount;
    return inlineComponentData[inlineComponentRootName].totalObjectCount;
}


static void processInlinComponentType(
        const QQmlType &type,
        qxp::function_ref<void(const QString&)> &&populateIcData)
{
    QString icRootName;
    if (type.isInlineComponentType()) {
        icRootName = type.elementName();
    }
    populateIcData(icRootName);
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
        if (const auto compilationUnit = typeRef->compilationUnit())
            qmlType = compilationUnit->qmlType;
        else
            qmlType = typeRef->type();
    }

    // Collect some data for instantiation later.
    using namespace icutils;
    std::vector<QV4::CompiledData::InlineComponent> allICs {};
    for (int i=0; i != objectCount(); ++i) {
        const CompiledObject *obj = objectAt(i);
        for (auto it = obj->inlineComponentsBegin(); it != obj->inlineComponentsEnd(); ++it) {
            allICs.push_back(*it);
        }
    }
    NodeList nodes;
    nodes.resize(allICs.size());
    std::iota(nodes.begin(), nodes.end(), 0);
    AdjacencyList adjacencyList;
    adjacencyList.resize(nodes.size());
    fillAdjacencyListForInlineComponents(this, adjacencyList, nodes, allICs);
    bool hasCycle = false;
    auto nodesSorted = topoSort(nodes, adjacencyList, hasCycle);
    Q_ASSERT(!hasCycle); // would have already been discovered by qqmlpropertycachcecreator

    // We need to first iterate over all inline components,
    // as the containing component might create instances of them
    // and in that case we need to add its object count
    for (auto nodeIt = nodesSorted.rbegin(); nodeIt != nodesSorted.rend(); ++nodeIt) {
        const auto &ic = allICs.at(nodeIt->index());
        const int lastICRoot = ic.objectIndex;
        for (int i = ic.objectIndex; i<objectCount(); ++i) {
            const QV4::CompiledData::Object *obj = objectAt(i);
            bool leftCurrentInlineComponent
                    = (i != lastICRoot
                       && obj->hasFlag(QV4::CompiledData::Object::IsInlineComponentRoot))
                    || !obj->hasFlag(QV4::CompiledData::Object::IsPartOfInlineComponent);
            if (leftCurrentInlineComponent)
                break;
            const QString lastICRootName = stringAt(ic.nameIndex);
            inlineComponentData[lastICRootName].totalBindingCount
                    += obj->nBindings;

            if (auto *typeRef = resolvedTypes.value(obj->inheritedTypeNameIndex)) {
                const auto type = typeRef->type();
                if (type.isValid() && type.parserStatusCast() != -1)
                    ++inlineComponentData[lastICRootName].totalParserStatusCount;

                ++inlineComponentData[lastICRootName].totalObjectCount;
                if (const auto compilationUnit = typeRef->compilationUnit()) {
                    // if the type is an inline component type, we have to extract the information
                    // from it.
                    // This requires that inline components are visited in the correct order.
                    processInlinComponentType(type, [&](const QString &currentlyVisitedICName) {
                        auto &icData = inlineComponentData[lastICRootName];
                        icData.totalBindingCount += compilationUnit->totalBindingsCount(currentlyVisitedICName);
                        icData.totalParserStatusCount += compilationUnit->totalParserStatusCount(currentlyVisitedICName);
                        icData.totalObjectCount += compilationUnit->totalObjectCount(currentlyVisitedICName);
                    });
                }
            }
        }
    }
    int bindingCount = 0;
    int parserStatusCount = 0;
    int objectCount = 0;
    for (quint32 i = 0, count = this->objectCount(); i < count; ++i) {
        const QV4::CompiledData::Object *obj = objectAt(i);
        if (obj->hasFlag(QV4::CompiledData::Object::IsPartOfInlineComponent))
            continue;

        bindingCount += obj->nBindings;
        if (auto *typeRef = resolvedTypes.value(obj->inheritedTypeNameIndex)) {
            const auto type = typeRef->type();
            if (type.isValid() && type.parserStatusCast() != -1)
                ++parserStatusCount;
            ++objectCount;
            if (const auto compilationUnit = typeRef->compilationUnit()) {
                processInlinComponentType(type, [&](const QString &currentlyVisitedICName){
                    bindingCount += compilationUnit->totalBindingsCount(currentlyVisitedICName);
                    parserStatusCount += compilationUnit->totalParserStatusCount(currentlyVisitedICName);
                    objectCount += compilationUnit->totalObjectCount(currentlyVisitedICName);
                });
            }
        }
    }

    m_totalBindingsCount = bindingCount;
    m_totalParserStatusCount = parserStatusCount;
    m_totalObjectCount = objectCount;
}

int CompilationUnit::totalParserStatusCount(const QString &inlineComponentRootName) const
{
    if (inlineComponentRootName.isEmpty())
        return m_totalParserStatusCount;
    return inlineComponentData[inlineComponentRootName].totalParserStatusCount;
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
