/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#ifndef INVISIBLE_H
#define INVISIBLE_H

#include <QtCore/qobject.h>
#include <QtQmlIntegration/qqmlintegration.h>

class Invisible : public QObject
{
public:
    Invisible(QObject *parent = nullptr) : QObject(parent) {}
};

class SingletonModel : public Invisible
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    SingletonModel(QObject *parent = nullptr) : Invisible(parent) {}
};

class AttachedAttached : public Invisible
{
    Q_OBJECT
public:
    AttachedAttached(QObject *parent = nullptr) : Invisible(parent) {}
};

class AttachedObject : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_ATTACHED(AttachedAttached)
public:
    static AttachedAttached *qmlAttachedProperties(QObject *object)
    {
        return new AttachedAttached(object);
    }
};

class DerivedFromInvisible : public Invisible
{
    Q_OBJECT
    QML_ELEMENT
};

#endif // INVISIBLE_H
