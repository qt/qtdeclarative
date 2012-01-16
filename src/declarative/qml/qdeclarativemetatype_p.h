/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QDECLARATIVEMETATYPE_P_H
#define QDECLARATIVEMETATYPE_P_H

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

#include "qdeclarative.h"

#include <QtCore/qglobal.h>
#include <QtCore/qvariant.h>
#include <QtCore/qbitarray.h>
#include <private/qdeclarativeglobal_p.h>
#include <QtDeclarative/qjsvalue.h>

QT_BEGIN_NAMESPACE

class QDeclarativeType;
class QDeclarativeCustomParser;
class QDeclarativeTypePrivate;
class QDeclarativeTypeModule;

class Q_DECLARATIVE_PRIVATE_EXPORT QDeclarativeMetaType
{
public:
    static QList<QString> qmlTypeNames();
    static QList<QDeclarativeType*> qmlTypes();

    static QDeclarativeType *qmlType(const QString &, int, int);
    static QDeclarativeType *qmlType(const QMetaObject *);
    static QDeclarativeType *qmlType(const QMetaObject *metaObject, const QString &module, int version_major, int version_minor);
    static QDeclarativeType *qmlType(int);

    static QMetaProperty defaultProperty(const QMetaObject *);
    static QMetaProperty defaultProperty(QObject *);
    static QMetaMethod defaultMethod(const QMetaObject *);
    static QMetaMethod defaultMethod(QObject *);

    static bool isQObject(int);
    static QObject *toQObject(const QVariant &, bool *ok = 0);

    static int listType(int);
    static int attachedPropertiesFuncId(const QMetaObject *);
    static QDeclarativeAttachedPropertiesFunc attachedPropertiesFuncById(int);

    enum TypeCategory { Unknown, Object, List };
    static TypeCategory typeCategory(int);
        
    static bool isInterface(int);
    static const char *interfaceIId(int);
    static bool isList(int);

    typedef QVariant (*StringConverter)(const QString &);
    static void registerCustomStringConverter(int, StringConverter);
    static StringConverter customStringConverter(int);

    static bool isAnyModule(const QString &uri);
    static bool isModule(const QString &module, int versionMajor, int versionMinor);
    static QDeclarativeTypeModule *typeModule(const QString &uri, int majorVersion);

    static QList<QDeclarativePrivate::AutoParentFunction> parentFunctions();

    static int QQuickAnchorLineMetaTypeId();
    typedef bool (*CompareFunction)(const void *, const void *);
    static void setQQuickAnchorLineCompareFunction(CompareFunction);
    static bool QQuickAnchorLineCompare(const void *p1, const void *p2);

    struct ModuleApiInstance {
        ModuleApiInstance()
            : scriptCallback(0), qobjectCallback(0), qobjectApi(0) {}

        QJSValue (*scriptCallback)(QDeclarativeEngine *, QJSEngine *);
        QObject *(*qobjectCallback)(QDeclarativeEngine *, QJSEngine *);
        QJSValue scriptApi;
        QObject *qobjectApi;
    };
    struct ModuleApi {
        inline ModuleApi();
        inline bool operator==(const ModuleApi &) const;
        int major;
        int minor;
        QJSValue (*script)(QDeclarativeEngine *, QJSEngine *);
        QObject *(*qobject)(QDeclarativeEngine *, QJSEngine *);
    };
    static ModuleApi moduleApi(const QString &, int, int);
    static QHash<QString, QList<ModuleApi> > moduleApis();

private:
    static CompareFunction anchorLineCompareFunction;
};

class QHashedStringRef;
class QHashedV8String;
class Q_DECLARATIVE_PRIVATE_EXPORT QDeclarativeType
{
public:
    QByteArray typeName() const;
    const QString &qmlTypeName() const;
    const QString &elementName() const;

    QString module() const;
    int majorVersion() const;
    int minorVersion() const;

    bool availableInVersion(int vmajor, int vminor) const;
    bool availableInVersion(const QString &module, int vmajor, int vminor) const;

    QObject *create() const;
    void create(QObject **, void **, size_t) const;

    typedef void (*CreateFunc)(void *);
    CreateFunc createFunction() const;
    int createSize() const;

    QDeclarativeCustomParser *customParser() const;

    bool isCreatable() const;
    bool isExtendedType() const;
    QString noCreationReason() const;

    bool isInterface() const;
    int typeId() const;
    int qListTypeId() const;

    const QMetaObject *metaObject() const;
    const QMetaObject *baseMetaObject() const;
    int metaObjectRevision() const;
    bool containsRevisionedAttributes() const;

    QDeclarativeAttachedPropertiesFunc attachedPropertiesFunction() const;
    const QMetaObject *attachedPropertiesType() const;
    int attachedPropertiesId() const;

    int parserStatusCast() const;
    QVariant fromObject(QObject *) const;
    const char *interfaceIId() const;
    int propertyValueSourceCast() const;
    int propertyValueInterceptorCast() const;

    int index() const;

    int enumValue(const QHashedStringRef &) const;
    int enumValue(const QHashedV8String &) const;
private:
    QDeclarativeType *superType() const;
    friend class QDeclarativeTypePrivate;
    friend struct QDeclarativeMetaTypeData;
    friend int registerType(const QDeclarativePrivate::RegisterType &);
    friend int registerInterface(const QDeclarativePrivate::RegisterInterface &);
    QDeclarativeType(int, const QDeclarativePrivate::RegisterInterface &);
    QDeclarativeType(int, const QDeclarativePrivate::RegisterType &);
    ~QDeclarativeType();

    QDeclarativeTypePrivate *d;
};

class QDeclarativeTypeModulePrivate;
class QDeclarativeTypeModule
{
public:
    QString module() const;
    int majorVersion() const;

    int minimumMinorVersion() const;
    int maximumMinorVersion() const;

    QList<QDeclarativeType *> types();
    QList<QDeclarativeType *> type(const QString &);

    QDeclarativeType *type(const QHashedStringRef &, int);
    QDeclarativeType *type(const QHashedV8String &, int);

private:
    friend int registerType(const QDeclarativePrivate::RegisterType &);
    QDeclarativeTypeModule();
    ~QDeclarativeTypeModule();
    QDeclarativeTypeModulePrivate *d;
};

class QDeclarativeTypeModuleVersion 
{
public:
    QDeclarativeTypeModuleVersion();
    QDeclarativeTypeModuleVersion(QDeclarativeTypeModule *, int);
    QDeclarativeTypeModuleVersion(const QDeclarativeTypeModuleVersion &);
    QDeclarativeTypeModuleVersion &operator=(const QDeclarativeTypeModuleVersion &);

    QDeclarativeTypeModule *module() const;
    int minorVersion() const;

    QDeclarativeType *type(const QHashedStringRef &) const;
    QDeclarativeType *type(const QHashedV8String &) const;

private:
    QDeclarativeTypeModule *m_module;
    int m_minor;
};

QDeclarativeMetaType::ModuleApi::ModuleApi()
{
    major = 0;
    minor = 0;
    script = 0;
    qobject = 0;
}

bool QDeclarativeMetaType::ModuleApi::operator==(const ModuleApi &other) const
{
    return major == other.major && minor == other.minor && script == other.script && qobject == other.qobject;
}

inline uint qHash(const QDeclarativeMetaType::ModuleApi &import)
{
    return import.major ^ import.minor ^ quintptr(import.script) ^ quintptr(import.qobject);
}

QT_END_NAMESPACE

#endif // QDECLARATIVEMETATYPE_P_H

