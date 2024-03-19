// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLBUILTINS_H
#define QQMLBUILTINS_H

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

// QmlBuiltins does not link QtQml - rather the other way around. Still, we can use the QtQml
// headers here. This works because we explicitly include the QtQml include directories in the
// manual moc call.
#include <private/qqmlcomponentattached_p.h>
#include <QtQml/qjsvalue.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlscriptstring.h>

#include <QtQmlIntegration/qqmlintegration.h>

#include <QtCore/qobject.h>
#include <QtCore/qglobal.h>
#include <QtCore/qtmetamacros.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qstring.h>
#include <QtCore/qurl.h>
#include <QtCore/qvariantmap.h>
#include <QtCore/qtypes.h>
#include <QtCore/qchar.h>

#if QT_CONFIG(regularexpression)
#include <QtCore/qregularexpression.h>
#endif

QT_BEGIN_NAMESPACE

#define QML_EXTENDED_JAVASCRIPT(EXTENDED_TYPE) \
    Q_CLASSINFO("QML.Extended", #EXTENDED_TYPE) \
    Q_CLASSINFO("QML.ExtensionIsJavaScript", "true") \

struct QQmlVoidForeign
{
    Q_GADGET
    QML_VALUE_TYPE(void)
    QML_EXTENDED_JAVASCRIPT(undefined)
#if !QT_CONFIG(regularexpression)
    QML_VALUE_TYPE(regexp)
#endif
    QML_FOREIGN(void)
};

struct QQmlVarForeign
{
    Q_GADGET
    QML_VALUE_TYPE(var)
    QML_VALUE_TYPE(variant)
    QML_FOREIGN(QVariant)
    QML_EXTENDED(QQmlVarForeign)
};

struct QQmlQtObjectForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(QtObject)
    QML_EXTENDED_JAVASCRIPT(Object)
    QML_FOREIGN(QObject)
    Q_CLASSINFO("QML.Root", "true")
};

struct QQmlIntForeign
{
    Q_GADGET
    QML_VALUE_TYPE(int)
    QML_EXTENDED_JAVASCRIPT(Number)
    QML_FOREIGN(int)
};

struct QQmlDoubleForeign
{
    Q_GADGET
    QML_VALUE_TYPE(real)
    QML_VALUE_TYPE(double)
    QML_EXTENDED_JAVASCRIPT(Number)
    QML_FOREIGN(double)
};

struct QQmlStringForeign
{
    Q_GADGET
    QML_VALUE_TYPE(string)
    QML_EXTENDED_JAVASCRIPT(String)
    QML_FOREIGN(QString)
};

struct QQmlBoolForeign
{
    Q_GADGET
    QML_VALUE_TYPE(bool)
    QML_EXTENDED_JAVASCRIPT(Boolean)
    QML_FOREIGN(bool)
};

struct QQmlDateForeign
{
    Q_GADGET
    QML_VALUE_TYPE(date)
    QML_EXTENDED_JAVASCRIPT(Date)
    QML_FOREIGN(QDateTime)
};

struct QQmlUrlForeign
{
    Q_GADGET
    QML_VALUE_TYPE(url)
    QML_EXTENDED_JAVASCRIPT(URL)
    QML_FOREIGN(QUrl)
};

#if QT_CONFIG(regularexpression)
struct QQmlRegexpForeign
{
    Q_GADGET
    QML_VALUE_TYPE(regexp)
    QML_EXTENDED_JAVASCRIPT(RegExp)
    QML_FOREIGN(QRegularExpression)
};
#endif

struct QQmlNullForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(std::nullptr_t)
    QML_EXTENDED(QQmlNullForeign)
};

struct QQmlQVariantMapForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QVariantMap)
    QML_EXTENDED_JAVASCRIPT(Object)
};

struct QQmlQint8Foreign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_EXTENDED_JAVASCRIPT(Number)
    QML_FOREIGN(qint8)
};

struct QQmlQuint8Foreign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_EXTENDED_JAVASCRIPT(Number)
    QML_FOREIGN(quint8)
};

struct QQmlShortForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_EXTENDED_JAVASCRIPT(Number)
    QML_FOREIGN(short)
};

struct QQmlUshortForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_EXTENDED_JAVASCRIPT(Number)
    QML_FOREIGN(ushort)
};

struct QQmlUintForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_EXTENDED_JAVASCRIPT(Number)
    QML_FOREIGN(uint)
};

struct QQmlQlonglongForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_EXTENDED_JAVASCRIPT(Number)
    QML_FOREIGN(qlonglong)
};

struct QQmlQulonglongForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_EXTENDED_JAVASCRIPT(Number)
    QML_FOREIGN(qulonglong)
};

struct QQmlFloatForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_EXTENDED_JAVASCRIPT(Number)
    QML_FOREIGN(float)
};

struct QQmlQCharForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QChar)
    QML_EXTENDED_JAVASCRIPT(String)
};

struct QQmlQDateForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QDate)
    QML_EXTENDED_JAVASCRIPT(Date)
};

struct QQmlQTimeForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QTime)
    QML_EXTENDED_JAVASCRIPT(Date)
};

struct QQmlQByteArrayForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_EXTENDED_JAVASCRIPT(ArrayBuffer)
    QML_FOREIGN(QByteArray)
};

struct QQmlQStringListForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QStringList)
    QML_SEQUENTIAL_CONTAINER(QString)
};

struct QQmlQVariantListForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QVariantList)
    QML_SEQUENTIAL_CONTAINER(QVariant)
};

struct QQmlQObjectListForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QObjectList)
    QML_SEQUENTIAL_CONTAINER(QObject *)
};

struct QQmlQJSValueForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QJSValue)
    QML_EXTENDED(QQmlQJSValueForeign)
};

struct QQmlComponentForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(Component)
    QML_FOREIGN(QQmlComponent)
    QML_ATTACHED(QQmlComponentAttached)
};

struct QQmlScriptStringForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QQmlScriptString)
};

QT_END_NAMESPACE

#endif // QQMLBUILTINS_H
