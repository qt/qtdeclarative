/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#ifndef TST_QMLTYPEREGISTRAR_H
#define TST_QMLTYPEREGISTRAR_H

#include "foreign.h"

#include <QtQml/qqml.h>
#include <QtCore/qproperty.h>
#include <QtCore/qtimeline.h>

class Interface {};
class Interface2 {};
class Interface3 {};

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(Interface, "io.qt.bugreports.Interface");
Q_DECLARE_INTERFACE(Interface2, "io.qt.bugreports.Interface2");
Q_DECLARE_INTERFACE(Interface3, "io.qt.bugreports.Interface3");
QT_END_NAMESPACE


class ImplementsInterfaces : public QObject, public Interface
{
    Q_OBJECT
    QML_ELEMENT
    QML_IMPLEMENTS_INTERFACES(Interface)
};

class ImplementsInterfaces2 : public QObject, public Interface, public Interface2
{
    Q_OBJECT
    QML_ELEMENT
    QML_IMPLEMENTS_INTERFACES(Interface Interface2)
};

class ExcessiveVersion : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int palette READ palette WRITE setPalette NOTIFY paletteChanged REVISION(6, 0))

public:
    int palette() const { return m_palette; }


    void setPalette(int palette)
    {
        if (m_palette == palette)
        return;

        m_palette = palette;
        emit paletteChanged();
    }

signals:
    Q_REVISION(6, 0) void paletteChanged();

private:
    int m_palette = 0;
};

class SizeEnums
{
    Q_GADGET
    QML_NAMED_ELEMENT(sizeEnums)
    QML_UNCREATABLE("Element is not creatable.")

public:
    enum Unit { Pixel, Centimeter, Inch, Point };
    Q_ENUM(Unit)
};

class SizeValueType : public SizeEnums
{
    QSize v;
    Q_GADGET
    Q_PROPERTY(int width READ width WRITE setWidth FINAL)
    QML_NAMED_ELEMENT(mySize)
    QML_FOREIGN(SizeGadget)
    QML_EXTENDED(SizeValueType)

public:
    Q_INVOKABLE QString sizeToString() const
    {
        return QString::fromLatin1("%1x%2").arg(v.width()).arg(v.height());
    }

    int width() const { return v.width(); }
    void setWidth(int width) { v.setWidth(width); }
};

class Local : public Foreign
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int someProperty MEMBER someProperty BINDABLE bindableSomeProperty)
public:
    enum Flag {
        Flag1 = 0x1,
        Flag2 = 0x2,
        Flag3 = 0x4,
        Flag4 = 0x8
    };
    Q_DECLARE_FLAGS(Flags, Flag)
    Q_FLAG(Flags)

    QBindable<int> bindableSomeProperty() {return QBindable<int>(&someProperty);}

    QProperty<int> someProperty;
};

namespace Namespace {
    class Element : public QObject
    {
        Q_OBJECT
        QML_ELEMENT
    };
}

class DerivedFromForeign : public QTimeLine
{
    Q_OBJECT
    QML_ELEMENT
public:
    DerivedFromForeign(QObject *parent) : QTimeLine(1000, parent) {}
};

class ExtensionA : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int a READ a CONSTANT)
public:
    ExtensionA(QObject *parent = nullptr) : QObject(parent) {}
    int a() const { return 'a'; }
};

class ExtensionB : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int b READ b CONSTANT)
public:
    ExtensionB(QObject *parent = nullptr) : QObject(parent) {}
    int b() const { return 'b'; }
};

class MultiExtensionParent : public QObject, public Interface3
{
    Q_OBJECT
    QML_ANONYMOUS
    QML_EXTENDED(ExtensionA)
    QML_IMPLEMENTS_INTERFACES(Interface3)
    Q_PROPERTY(int p READ p CONSTANT)
public:
    MultiExtensionParent(QObject *parent = nullptr) : QObject(parent) {}
    int p() const { return 'p'; }
};

class MultiExtension : public MultiExtensionParent
{
    Q_OBJECT
    QML_ELEMENT
    QML_EXTENDED(ExtensionB)
    Q_PROPERTY(int e READ e CONSTANT)
public:
    MultiExtension(QObject *parent = nullptr) : MultiExtensionParent(parent) {}
    int e() const { return 'e'; }
};

class tst_qmltyperegistrar : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void qmltypesHasForeign();
    void qmltypesHasHppClassAndNoext();
    void qmltypesHasReadAndWrite();
    void qmltypesHasFileNames();
    void qmltypesHasFlags();
    void superAndForeignTypes();
    void accessSemantics();
    void isBindable();
    void restrictToImportVersion();
    void pastMajorVersions();
    void implementsInterfaces();
    void namespacedElement();
    void derivedFromForeign();
    void metaTypesRegistered();
    void multiExtensions();

private:
    QByteArray qmltypesData;
};

#endif // TST_QMLTYPEREGISTRAR_H
