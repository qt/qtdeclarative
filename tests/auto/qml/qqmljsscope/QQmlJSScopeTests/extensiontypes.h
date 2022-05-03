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

#ifndef EXTENSIONTYPES_H
#define EXTENSIONTYPES_H

#include <QtCore/qobject.h>
#include <qqml.h>

class Extension : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int count READ getCount WRITE setCount NOTIFY countChanged)

public:
    Extension(QObject *parent = nullptr) : QObject(parent) { }
    int getCount() const { return 42; }
    void setCount(int) { }
Q_SIGNALS:
    void countChanged();
};

class IndirectExtension : public Extension
{
    Q_OBJECT
    QML_ANONYMOUS
public:
    IndirectExtension(QObject *parent = nullptr) : Extension(parent) { }
};

class Extended : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_EXTENDED(Extension)
    Q_PROPERTY(double count READ getCount WRITE setCount NOTIFY countChanged)

public:
    Extended(QObject *parent = nullptr) : QObject(parent) { }
    double getCount() const { return 0.0; }
    void setCount(double) { }
Q_SIGNALS:
    void countChanged();
};

class ExtendedIndirect : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_EXTENDED(IndirectExtension)
    Q_PROPERTY(double count READ getCount WRITE setCount NOTIFY countChanged)

public:
    ExtendedIndirect(QObject *parent = nullptr) : QObject(parent) { }
    double getCount() const { return 0; }
    void setCount(double) { }
Q_SIGNALS:
    void countChanged();
};

class Extension2 : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(QString str READ getStr WRITE setStr NOTIFY strChanged)

public:
    Extension2(QObject *parent = nullptr) : QObject(parent) { }
    QString getStr() const { return QStringLiteral("42"); }
    void setStr(QString) { }
Q_SIGNALS:
    void strChanged();
};

class ExtendedTwice : public Extended
{
    Q_OBJECT
    QML_ELEMENT
    QML_EXTENDED(Extension2)
    Q_PROPERTY(QByteArray str READ getStr WRITE setStr)

public:
    ExtendedTwice(QObject *parent = nullptr) : Extended(parent) { }
    QByteArray getStr() const { return QByteArray(); }
    void setStr(QByteArray) { }
};

#endif // EXTENSIONTYPES_H
