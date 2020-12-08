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

#ifndef QQMLMETATYPE_P_H
#define QQMLMETATYPE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qtqmlglobal_p.h>
#include <private/qqmltype_p.h>
#include <private/qqmlproxymetaobject_p.h>
#include <private/qqmldirparser_p.h>

QT_BEGIN_NAMESPACE

class QQmlTypeModule;
class QRecursiveMutex;
class QQmlError;

namespace QV4 { class ExecutableCompilationUnit; }

struct CompositeMetaTypeIds
{
private:
    int *refCount = nullptr;
    void deref();
    void ref()
    {
        Q_ASSERT(refCount);
        ++*refCount;
    }
public:
    CompositeMetaTypeIds() = default;
    CompositeMetaTypeIds(QMetaType id, QMetaType listId) : id(id), listId(listId) {}
    CompositeMetaTypeIds(const CompositeMetaTypeIds &other)
        : refCount(other.refCount), id(other.id), listId(other.listId)
    {
        if (refCount)
            ref();
    }
    CompositeMetaTypeIds(CompositeMetaTypeIds &&other)
        : refCount(other.refCount), id(other.id), listId(other.listId)
    {
        other.refCount = nullptr;
    }
    CompositeMetaTypeIds &operator=(const CompositeMetaTypeIds &other)
    {
        if (refCount)
            deref();
        refCount = other.refCount;
        id = other.id;
        listId = other.listId;
        if (refCount)
            ref();
        return *this;
    }
    CompositeMetaTypeIds &operator=(CompositeMetaTypeIds &&other)
    {
        if (refCount)
            deref();
        refCount = other.refCount;
        id = other.id;
        listId = other.listId;
        other.refCount = nullptr;
        return *this;
    }
    ~CompositeMetaTypeIds();
    static CompositeMetaTypeIds fromCompositeName(const QByteArray &name);
public:
    QMetaType id;
    QMetaType listId;
    bool isValid() const { return id.isValid() && listId.isValid(); }
};

class Q_QML_PRIVATE_EXPORT QQmlMetaType
{
    friend struct CompositeMetaTypeIds;
    static CompositeMetaTypeIds registerInternalCompositeType(const QByteArray &className);
    static void unregisterInternalCompositeType(const CompositeMetaTypeIds &typeIds);

public:
    enum class RegistrationResult {
        Success,
        Failure,
        NoRegistrationFunction
    };

    static QQmlType registerType(const QQmlPrivate::RegisterType &type);
    static QQmlType registerInterface(const QQmlPrivate::RegisterInterface &type);
    static QQmlType registerSingletonType(const QQmlPrivate::RegisterSingletonType &type);
    static QQmlType registerCompositeSingletonType(const QQmlPrivate::RegisterCompositeSingletonType &type);
    static QQmlType registerCompositeType(const QQmlPrivate::RegisterCompositeType &type);
    static RegistrationResult registerPluginTypes(QObject *instance, const QString &basePath,
                                                  const QString &uri, const QString &typeNamespace,
                                                  QTypeRevision version, QList<QQmlError> *errors);
    static QQmlType typeForUrl(const QString &urlString, const QHashedStringRef& typeName,
                               bool isCompositeSingleton, QList<QQmlError> *errors,
                               QTypeRevision version = QTypeRevision());

    static void unregisterType(int type);

    static void registerModule(const char *uri, QTypeRevision version);
    static bool protectModule(const QString &uri, QTypeRevision version,
                              bool protectAllVersions = false);

    static void registerModuleImport(const QString &uri, QTypeRevision version,
                                     const QQmlDirParser::Import &import);
    static void unregisterModuleImport(const QString &uri, QTypeRevision version,
                                       const QQmlDirParser::Import &import);
    static QList<QQmlDirParser::Import> moduleImports(const QString &uri, QTypeRevision version);

    static int typeId(const char *uri, QTypeRevision version, const char *qmlName);

    static void registerUndeletableType(const QQmlType &dtype);

    static QList<QString> qmlTypeNames();
    static QList<QQmlType> qmlTypes();
    static QList<QQmlType> qmlSingletonTypes();
    static QList<QQmlType> qmlAllTypes();

    enum class TypeIdCategory {
        MetaType,
        QmlType
    };

    static QQmlType qmlType(const QString &qualifiedName, QTypeRevision version);
    static QQmlType qmlType(const QHashedStringRef &name, const QHashedStringRef &module, QTypeRevision version);
    static QQmlType qmlType(const QMetaObject *);
    static QQmlType qmlType(const QMetaObject *metaObject, const QHashedStringRef &module, QTypeRevision version);
    static QQmlType qmlType(int typeId, TypeIdCategory category = TypeIdCategory::MetaType);
    static QQmlType qmlType(const QUrl &unNormalizedUrl, bool includeNonFileImports = false);

    static QQmlPropertyCache *propertyCache(const QMetaObject *metaObject,
                                            QTypeRevision version = QTypeRevision(), bool doRef = false);
    static QQmlPropertyCache *propertyCache(const QQmlType &type, QTypeRevision version);

    static void freeUnusedTypesAndCaches();

    static QMetaProperty defaultProperty(const QMetaObject *);
    static QMetaProperty defaultProperty(QObject *);
    static QMetaMethod defaultMethod(const QMetaObject *);
    static QMetaMethod defaultMethod(QObject *);

    static bool isQObject(int);
    static QObject *toQObject(const QVariant &, bool *ok = nullptr);

    static int listType(int);
    static QQmlAttachedPropertiesFunc attachedPropertiesFunc(QQmlEnginePrivate *,
                                                             const QMetaObject *);

    enum TypeCategory { Unknown, Object, List };
    static TypeCategory typeCategory(int);

    static bool isInterface(int);
    static const char *interfaceIId(int);
    static bool isList(int);

    typedef QVariant (*StringConverter)(const QString &);
    static void registerCustomStringConverter(int, StringConverter);
    static StringConverter customStringConverter(int);

    static QTypeRevision latestModuleVersion(const QString &uri);
    static bool isLockedModule(const QString &uri, QTypeRevision version);
    static QTypeRevision matchingModuleVersion(const QString &module, QTypeRevision version);
    static QQmlTypeModule *typeModule(const QString &uri, QTypeRevision version);

    static QList<QQmlPrivate::AutoParentFunction> parentFunctions();

    enum class CachedUnitLookupError {
        NoError,
        NoUnitFound,
        VersionMismatch
    };

    static const QQmlPrivate::CachedQmlUnit *findCachedCompilationUnit(const QUrl &uri, CachedUnitLookupError *status);

    // used by tst_qqmlcachegen.cpp
    static void prependCachedUnitLookupFunction(QQmlPrivate::QmlUnitCacheLookupFunction handler);
    static void removeCachedUnitLookupFunction(QQmlPrivate::QmlUnitCacheLookupFunction handler);

    static QRecursiveMutex *typeRegistrationLock();

    static QString prettyTypeName(const QObject *object);

    template <typename QQmlTypeContainer>
    static void removeQQmlTypePrivate(QQmlTypeContainer &container,
                                      const QQmlTypePrivate *reference)
    {
        for (typename QQmlTypeContainer::iterator it = container.begin(); it != container.end();) {
            if (*it == reference)
                it = container.erase(it);
            else
                ++it;
        }
    }

    static int registerAutoParentFunction(const QQmlPrivate::RegisterAutoParent &autoparent);
    static void unregisterAutoParentFunction(const QQmlPrivate::AutoParentFunction &function);

    static QQmlType registerSequentialContainer(
            const QQmlPrivate::RegisterSequentialContainer &sequenceRegistration);
    static void unregisterSequentialContainer(int id);

    static int registerUnitCacheHook(const QQmlPrivate::RegisterQmlUnitCacheHook &hookRegistration);
    static void clearTypeRegistrations();

    static QList<QQmlProxyMetaObject::ProxyData> proxyData(const QMetaObject *mo,
                                                           const QMetaObject *baseMetaObject,
                                                           QMetaObject *lastMetaObject);

    static void clone(QMetaObjectBuilder &builder, const QMetaObject *mo,
                      const QMetaObject *ignoreStart, const QMetaObject *ignoreEnd);

    static void qmlInsertModuleRegistration(const QString &uri, void (*registerFunction)());
    static void qmlRemoveModuleRegistration(const QString &uri);

    static bool qmlRegisterModuleTypes(const QString &uri);

    static int qmlRegisteredListTypeCount();
};

Q_DECLARE_TYPEINFO(QQmlMetaType, Q_RELOCATABLE_TYPE);

QT_END_NAMESPACE

#endif // QQMLMETATYPE_P_H

