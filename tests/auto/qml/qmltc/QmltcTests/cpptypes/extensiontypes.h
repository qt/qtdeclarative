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
#include <QtCore/qproperty.h>
#include <qqml.h>

class Extension : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int count READ getCount WRITE setCount BINDABLE bindableCount)
    Q_PROPERTY(double foo READ getFoo WRITE setFoo BINDABLE bindableFoo)

    QProperty<int> m_extCount { 0 };
    QProperty<double> m_foo { 0 };

public:
    Extension(QObject *parent = nullptr);
    int getCount() const;
    void setCount(int v);
    QBindable<int> bindableCount();

    double getFoo() const;
    void setFoo(double v);
    QBindable<double> bindableFoo();
};

class IndirectExtension : public Extension
{
    Q_OBJECT
    QML_ANONYMOUS
public:
    IndirectExtension(QObject *parent = nullptr);
};

class TypeWithExtension : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int count READ getCount WRITE setCount BINDABLE bindableCount)
    QML_EXTENDED(Extension)

    QProperty<int> m_count;

public:
    TypeWithExtension(QObject *parent = nullptr);
    int getCount() const;
    void setCount(int v);
    QBindable<int> bindableCount();

    static constexpr int unsetCount = 100;
};

class Extension2 : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(QString str READ getStr WRITE setStr BINDABLE bindableStr)

    QProperty<QString> m_extStr {};

public:
    Extension2(QObject *parent = nullptr);

    QString getStr() const;
    void setStr(QString v);
    QBindable<QString> bindableStr();
};

class TypeWithExtensionDerived : public TypeWithExtension
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString str READ getStr WRITE setStr BINDABLE bindableStr)
    QML_EXTENDED(Extension2)

    QProperty<QString> m_str;

public:
    TypeWithExtensionDerived(QObject *parent = nullptr);

    QString getStr() const;
    void setStr(QString v);
    QBindable<QString> bindableStr();

    static const QString unsetStr;
};

class TypeWithExtensionNamespace : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int count READ getCount WRITE setCount BINDABLE bindableCount)
    QML_EXTENDED_NAMESPACE(Extension)

    QProperty<int> m_count;

public:
    TypeWithExtensionNamespace(QObject *parent = nullptr);

    int getCount() const;
    void setCount(int v);
    QBindable<int> bindableCount();
};

class TypeWithBaseTypeExtension : public TypeWithExtensionDerived
{
    Q_OBJECT
    QML_ELEMENT

public:
    TypeWithBaseTypeExtension(QObject *parent = nullptr) : TypeWithExtensionDerived(parent) { }
};

#endif // EXTENSIONTYPES_H
