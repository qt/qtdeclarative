// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQMLTCPROPERTYUTILS_P_H
#define QQMLTCPROPERTYUTILS_P_H

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

#include <private/qqmljsmetatypes_p.h>
#include <private/qqmljsscope_p.h>
#include <QtQml/private/qqmlsignalnames_p.h>

QT_BEGIN_NAMESPACE

namespace QQmltc {

/*!
    \internal

    Returns an underlying C++ type of \a p property.
*/
inline QString getUnderlyingType(const QQmlJSMetaProperty &p)
{
    if (p.isList()) {
        // We cannot just use p.type()->internalName() here because it may be
        // a list property of something that only receives a C++ name from qmltc.
        const QQmlJSScope::ConstPtr elementType = p.type()->elementType();
        return (elementType->isReferenceType() ? u"QQmlListProperty<" : u"QList<")
                + elementType->internalName() + u'>';
    }

    return p.type()->augmentedInternalName();
}

// simple class that, for a given property, creates information for the
// Q_PROPERTY macro (READ/WRITE function names, etc.)
struct PropertyData
{
    PropertyData(const QQmlJSMetaProperty &p) : PropertyData(p.propertyName()) { }

    PropertyData(const QString &propertyName)
    {
        read = propertyName;
        write = QQmlSignalNames::addPrefixToPropertyName(u"set", propertyName);
        bindable = QQmlSignalNames::addPrefixToPropertyName(u"bindable", propertyName);
        notify = QQmlSignalNames::propertyNameToChangedSignalName(propertyName);
        reset = QQmlSignalNames::addPrefixToPropertyName(u"reset", propertyName);
    }

    QString read;
    QString write;
    QString bindable;
    QString notify;
    QString reset;
};

} // namespace QQmltc

QT_END_NAMESPACE

#endif // QQMLTCPROPERTYUTILS_P_H
