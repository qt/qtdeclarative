/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef TESTCASE_H
#define TESTCASE_H

// This is a dummy header for defining the interface of "TestCase.qml" to qdoc.

#include <QtQuick/qquickitem.h>

QT_BEGIN_NAMESPACE

class TestCase : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(bool when READ when WRITE setWhen NOTIFY whenChanged)
    Q_PROPERTY(bool optional READ optional WRITE setOptional NOTIFY optionalChanged)
    Q_PROPERTY(bool completed READ completed NOTIFY completedChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(bool windowShown READ windowShown NOTIFY windowShownChanged)
public:
    TestCase(QQuickItem *parent) : QQuickItem(parent) {}
    ~TestCase()

    QString name() const;
    void setName(const QString &name);

    bool when() const;
    void setWhen(bool when);

    bool optional() const;
    void setOptional(bool optional);

    bool completed() const;
    bool running() const;
    bool windowShown() const;

Q_SIGNALS:
    void nameChanged();
    void whenChanged();
    void optionalChanged();
    void completedChanged();
    void runningChanged();
    void windowShownChanged();
};

QML_DECLARE_TYPE(TestCase)

QT_END_NAMESPACE

#endif
