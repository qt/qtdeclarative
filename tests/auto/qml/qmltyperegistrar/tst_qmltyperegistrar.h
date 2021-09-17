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
#include "foreign_p.h"

#include <QtQml/qqml.h>
#include <QtQml/qqmlcomponent.h>
#include <QtCore/qproperty.h>
#include <QtCore/qtimeline.h>
#include <QtCore/qrect.h>

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

class ForeignWithoutDefault : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject *d READ d WRITE setD NOTIFY dChanged)
public:
    QObject *d() const { return m_d; }

    void setD(QObject *d)
    {
        if (m_d != d) {
            m_d = d;
            emit dChanged();
        }
    }

signals:
    void dChanged();

private:
    QObject *m_d = nullptr;
};

class LocalWithDefault : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject *d READ d WRITE setD NOTIFY dChanged)
    QML_NAMED_ELEMENT(ForeignWithoutDefault)
    QML_FOREIGN(ForeignWithoutDefault)
    Q_CLASSINFO("DefaultProperty", "d")

public:
    LocalWithDefault(QObject *parent = nullptr) : QObject(parent) {}
    QObject *d() const { return m_d; }

    void setD(QObject *d)
    {
        if (m_d != d) {
            m_d = d;
            emit dChanged();
        }
    }

signals:
    void dChanged();

private:
    QObject *m_d = nullptr;
};

class Local : public Foreign
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int someProperty MEMBER someProperty BINDABLE bindableSomeProperty)
    QML_EXTENDED(LocalWithDefault)
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

class RequiredProperty : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int foo READ foo WRITE setFoo REQUIRED)
public:
    RequiredProperty(QObject *parent = nullptr) : QObject(parent) {}
    int foo() { return m_foo; }
    void setFoo(int foo) { m_foo = foo; }
private:
    int m_foo;
};

class FinalProperty : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int fff MEMBER fff FINAL)
public:
    int fff = 0;
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

class HiddenAccessorsPrivate
{
public:
    QString hiddenRead() const { return QStringLiteral("bar"); }
};

class HiddenAccessors : public QObject
{
    Q_OBJECT
    Q_PRIVATE_PROPERTY(HiddenAccessors::d_func(), QString hiddenRead READ hiddenRead CONSTANT)
    QML_ELEMENT
    Q_DECLARE_PRIVATE(HiddenAccessors)
};

struct SelfExtensionHack
{
    QRectF rect;
    Q_GADGET
    QML_EXTENDED(SelfExtensionHack)
    QML_FOREIGN(QRectF)
    QML_VALUE_TYPE(recterei)
};

class ParentProperty : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject *ppp READ ppp BINDABLE pppBindable)
    QML_ELEMENT
    Q_CLASSINFO("ParentProperty", "ppp")
public:

    QObject *ppp() const { return m_parent.value(); }
    QBindable<QObject *> pppBindable() { return QBindable<QObject *>(&m_parent); }

private:
    QProperty<QObject *> m_parent;
};


class ValueTypeWithEnum1
{
    Q_GADGET
    Q_PROPERTY(ValueTypeWithEnum1::Quality quality READ quality WRITE setQuality)
public:
    enum Quality
    {
        VeryLowQuality,
        LowQuality,
        NormalQuality,
        HighQuality,
        VeryHighQuality
    };
    Q_ENUM(Quality)

    Quality quality() const { return m_quality; }
    void setQuality(Quality quality) { m_quality = quality; }

private:
    Quality m_quality = HighQuality;
};

// value type alphabetically first
struct AValueTypeWithEnumForeign1
{
    Q_GADGET
    QML_FOREIGN(ValueTypeWithEnum1)
    QML_NAMED_ELEMENT(valueTypeWithEnum1)
};

// namespace alphabetically second
namespace BValueTypeWithEnumForeignNamespace1
{
    Q_NAMESPACE
    QML_FOREIGN_NAMESPACE(ValueTypeWithEnum1)
    QML_NAMED_ELEMENT(ValueTypeWithEnum1)
};

class ValueTypeWithEnum2
{
    Q_GADGET
    Q_PROPERTY(ValueTypeWithEnum2::Quality quality READ quality WRITE setQuality)
public:
    enum Quality
    {
        VeryLowQuality,
        LowQuality,
        NormalQuality,
        HighQuality,
        VeryHighQuality
    };
    Q_ENUM(Quality)

    Quality quality() const { return m_quality; }
    void setQuality(Quality quality) { m_quality = quality; }

private:
    Quality m_quality = HighQuality;
};

// namespace alphabetically first
namespace AValueTypeWithEnumForeignNamespace2
{
    Q_NAMESPACE
    QML_FOREIGN_NAMESPACE(ValueTypeWithEnum2)
    QML_NAMED_ELEMENT(ValueTypeWithEnum2)
};

// value type alphabetically second
struct BValueTypeWithEnumForeign2
{
    Q_GADGET
    QML_FOREIGN(ValueTypeWithEnum2)
    QML_NAMED_ELEMENT(valueTypeWithEnum2)
};

class DerivedFromForeignPrivate : public ForeignPrivate
{
    Q_OBJECT
    QML_ELEMENT
};

class WithMethod : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    Q_INVOKABLE QQmlComponent *createAThing(int) { return nullptr; }
};

class tst_qmltyperegistrar : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void qmltypesHasForeign();
    void qmltypesHasHppClassAndNoext();
    void qmltypesHasReadAndWrite();
    void qmltypesHasNotify();
    void qmltypesHasPropertyIndex();
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
    void localDefault();
    void requiredProperty();
    void hiddenAccessor();
    void finalProperty();
    void parentProperty();
    void namespacesAndValueTypes();
    void derivedFromForeignPrivate();
    void methodReturnType();

private:
    QByteArray qmltypesData;
};

#endif // TST_QMLTYPEREGISTRAR_H
