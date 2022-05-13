// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QMLTCPROPERTYUTILS_H
#define QMLTCPROPERTYUTILS_H

#include <private/qqmljsmetatypes_p.h>

QT_BEGIN_NAMESPACE

/*!
    \internal

    Returns an underlying C++ type of \a p property.
*/
inline QString getUnderlyingType(const QQmlJSMetaProperty &p)
{
    QString underlyingType = p.type()->internalName();
    // NB: can be a pointer or a list, can't be both (list automatically assumes
    // that it holds pointers though). check isList() first, as list<QtObject>
    // would be both a list and a pointer (weird).
    if (p.isList()) {
        underlyingType = u"QQmlListProperty<" + underlyingType + u">";
    } else if (p.type()->isReferenceType()) {
        underlyingType += u'*';
    }
    return underlyingType;
}

// simple class that, for a given property, creates information for the
// Q_PROPERTY macro (READ/WRITE function names, etc.)
struct QmltcPropertyData
{
    QmltcPropertyData(const QQmlJSMetaProperty &p) : QmltcPropertyData(p.propertyName()) { }

    QmltcPropertyData(const QString &propertyName)
    {
        const QString nameWithUppercase = propertyName[0].toUpper() + propertyName.sliced(1);

        read = propertyName;
        write = u"set" + nameWithUppercase;
        bindable = u"bindable" + nameWithUppercase;
        notify = propertyName + u"Changed";
    }

    QString read;
    QString write;
    QString bindable;
    QString notify;
};

QT_END_NAMESPACE

#endif // QMLTCPROPERTYUTILS_H
