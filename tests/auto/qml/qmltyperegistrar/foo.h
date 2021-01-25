/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#ifndef FOO_H
#define FOO_H

#include <QtCore/qobject.h>
#include <qqml.h>

class Bbb : public QObject
{
    Q_OBJECT
public:
    Bbb(QObject *parent = nullptr) : QObject(parent) {}
};

class Ccc : public QObject
{
    Q_OBJECT
public:
    Ccc(QObject *parent) : QObject(parent) {}
};

class Ooo : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_EXTENDED(Bbb)
    QML_ATTACHED(Ccc);
public:
    Q_INVOKABLE void blah();

    static Ccc *qmlAttachedProperties(QObject *o) { return new Ccc(o); }
};

namespace Foo {
Q_NAMESPACE
QML_ELEMENT

enum Bar {
    A, B, C
};

Q_ENUM_NS(Bar);
}


#endif // FOO_H
