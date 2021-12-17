/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QML2CPPPROPERTYUTILS_H
#define QML2CPPPROPERTYUTILS_H

#include <private/qqmljsmetatypes_p.h>

inline QString getUnderlyingType(const QQmlJSMetaProperty &p)
{
    QString underlyingType = p.type()->internalName();
    if (p.isList()) {
        underlyingType = u"QQmlListProperty<" + underlyingType + u">";
    } else if (p.type()->isReferenceType()) {
        underlyingType += u"*"_qs;
    }
    return underlyingType;
}

// dead simple class that, for a given property, creates information for
// the Q_PROPERTY macro (READ/WRITE function names, etc.)
struct Qml2CppPropertyData
{
    Qml2CppPropertyData(const QQmlJSMetaProperty &p) : Qml2CppPropertyData(p.propertyName()) { }

    Qml2CppPropertyData(const QString &propertyName)
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

#endif // QML2CPPPROPERTYUTILS_H
