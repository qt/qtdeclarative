// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QMLTCPROPERTYUTILS_H
#define QMLTCPROPERTYUTILS_H

#include <private/qqmljsmetatypes_p.h>
#include <private/qqmljsscope_p.h>

QT_BEGIN_NAMESPACE

/*!
    \internal

    Returns an underlying C++ type of \a p property.
*/
inline QString getUnderlyingType(const QQmlJSMetaProperty &p)
{
    if (p.isList()) {
        // We cannot just use p.type()->internalName() here because it may be
        // a list property of something that only receives a C++ name from qmltc.
        const QQmlJSScope::ConstPtr valueType = p.type()->valueType();
        return (valueType->isReferenceType() ? u"QQmlListProperty<" : u"QList<")
                + valueType->internalName() + u'>';
    }

    return p.type()->augmentedInternalName();
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
        reset = u"reset" + nameWithUppercase;
    }

    QString read;
    QString write;
    QString bindable;
    QString notify;
    QString reset;
};

QT_END_NAMESPACE

#endif // QMLTCPROPERTYUTILS_H
