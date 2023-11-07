// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef TST_QMLTYPEREGISTRAR_H
#define TST_QMLTYPEREGISTRAR_H

#include "foreign.h"
#include "foreign_p.h"

#include <QtQmlTypeRegistrar/private/qqmltyperegistrar_p.h>

#ifdef QT_QUICK_LIB
#    include <QtQuick/qquickitem.h>
#endif

#include <QtQml/qqml.h>
#include <QtQml/qqmlcomponent.h>

#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qproperty.h>
#include <QtCore/qrect.h>
#include <QtCore/qtemporaryfile.h>
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
    DerivedFromForeign(QObject *parent = nullptr) : QTimeLine(1000, parent) { }
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


namespace BaseNamespace
{
Q_NAMESPACE
enum BBB {
    D, E, F
};
Q_ENUM_NS(BBB)
}

struct ExtensionValueType
{
    Q_GADGET
public:
    enum EEE {
        A, B, C
    };
    Q_ENUM(EEE)
};

struct DeferredPropertyNamesEmpty : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DeferredPropertyNames", "")
};

struct DeferredPropertyNames : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DeferredPropertyNames", "A,B,C")
};

struct ImmediatePropertyNamesEmpty : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("ImmediatePropertyNames", "")
};

struct ImmediatePropertyNames : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("ImmediatePropertyNames", "A,B,C")
};

namespace ForeignNamespace
{
Q_NAMESPACE
QML_FOREIGN_NAMESPACE(BaseNamespace)
QML_NAMESPACE_EXTENDED(ExtensionValueType)
QML_ELEMENT
}

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

#ifdef QT_QUICK_LIB
class ForeignRevisionedProperty : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit ForeignRevisionedProperty(QQuickItem *parent = nullptr) : QQuickItem(parent) {};
};
#endif

class AddedInLateVersion : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Versioned)
    QML_ADDED_IN_VERSION(1, 8)
    Q_PROPERTY(int revisioned READ revisioned CONSTANT REVISION(1, 4))
    Q_PROPERTY(int insane READ revisioned CONSTANT REVISION 17)
public:
    AddedInLateVersion(QObject *parent = nullptr) : QObject(parent) {}
    int revisioned() const { return 24; }
};

class RemovedInEarlyVersion : public AddedInLateVersion
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Versioned)
    QML_ADDED_IN_VERSION(1, 3)
    QML_REMOVED_IN_VERSION(1, 8)
public:
    RemovedInEarlyVersion(QObject *parent = nullptr) : AddedInLateVersion(parent) {}
};

class HasResettableProperty : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int foo READ foo WRITE setFoo RESET resetFoo NOTIFY fooChanged)
public:
    HasResettableProperty(QObject *parent = nullptr) : QObject(parent) {}

    int foo() const { return m_foo; }
    void setFoo(int newFoo)
    {
        if (m_foo == newFoo)
            return;
        m_foo = newFoo;
        emit fooChanged();
    }
    void resetFoo() { setFoo(12); }

signals:
    void fooChanged();

private:
    int m_foo = 12;
};

class ClonedSignal : public QObject
{
    Q_OBJECT
    QML_ELEMENT
signals:
    void clonedSignal(int i = 7);
};

class Constructible
{
    Q_GADGET
    QML_VALUE_TYPE(constructible)
    QML_CONSTRUCTIBLE_VALUE
public:
    Q_INVOKABLE Constructible(int i = 12) : m_i(i) {}

private:
    int m_i;
};

class AnonymousAndUncreatable : public QObject
{
     Q_OBJECT
     QML_ANONYMOUS
     QML_UNCREATABLE("Pointless uncreatable message")
};

class Invisible : public QObject
{
};

struct InvisibleForeign
{
    Q_GADGET
    QML_FOREIGN(Invisible)
    QML_NAMED_ELEMENT(Invisible)
};

class TypedEnum : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    enum S: qint16 {
        A, B, C
    };
    Q_ENUM(S)

    enum T: quint16 {
        D, E, F
    };
    Q_ENUM(T)

    enum U: qint8 {
        G, H, I
    };
    Q_ENUM(U)

    enum V: quint8 {
        J, K, L
    };
    Q_ENUM(V)
};

class ListSignal : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS

Q_SIGNALS:
    void objectListHappened(const QList<QObject *> &);
};

class Bar : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int outerBarProp READ bar CONSTANT)
public:
    Bar(QObject *parent = nullptr) : QObject(parent) {}
    int bar() const { return 44; }
};

namespace Testing {

class Foo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int fooProp READ foo CONSTANT)

public:
    int foo() const { return 42; }
};

class Bar : public Foo
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int barProp READ bar CONSTANT)

public:
    int bar() const { return 43; }
};

namespace Inner {

class Baz : public Bar
{
    Q_OBJECT
    QML_ELEMENT

    QML_EXTENDED(::Bar)
    QML_ATTACHED(Foo)

public:
    static Foo *qmlAttachedProperties(QObject *) { return new Foo; }
};

} // namespace Inner
} // namespace Testing

struct QByteArrayStdVectorForeign
{
    Q_GADGET
    QML_ANONYMOUS
    QML_SEQUENTIAL_CONTAINER(QByteArray)
    QML_FOREIGN(std::vector<QByteArray>)
};

// Anonymous value type for an unknown foreign type
struct QPersistentModelIndexValueType
{
    QPersistentModelIndex v;
    Q_PROPERTY(int row READ row FINAL)
    Q_GADGET
    QML_ANONYMOUS
    QML_EXTENDED(QPersistentModelIndexValueType)
    QML_FOREIGN(QPersistentModelIndex)

public:
    inline int row() const { return v.row(); }
};


namespace NetworkManager {
Q_NAMESPACE

enum NM { A, B, C};
Q_ENUM_NS(NM)
}

struct NMForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(NetworkManager)
    QML_FOREIGN_NAMESPACE(NetworkManager)
};

struct NotNamespace {
    Q_GADGET
public:
    enum Abc {
        A, B, C, D
    };
    Q_ENUM(Abc);
};

struct NotNamespaceForeign {
    Q_GADGET
    QML_FOREIGN_NAMESPACE(NotNamespace)
    QML_ELEMENT
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
    void namespaceExtendedNamespace();
    void deferredNames();
    void immediateNames();
    void derivedFromForeignPrivate();
    void methodReturnType();
    void hasIsConstantInParameters();
    void uncreatable();
    void singletonVesions();

#ifdef QT_QUICK_LIB
    void foreignRevisionedProperty();
#endif

    void addRemoveVersion_data();
    void addRemoveVersion();
    void typeInModuleMajorVersionZero();
    void resettableProperty();
    void duplicateExportWarnings();
    void clonedSignal();
    void baseVersionInQmltypes();
    void constructibleValueType();
    void anonymousAndUncreatable();
    void omitInvisible();
    void typedEnum();
    void listSignal();
    void withNamespace();
    void sequenceRegistration();
    void valueTypeSelfReference();
    void foreignNamespaceFromGadget();

private:
    QByteArray qmltypesData;
};

#endif // TST_QMLTYPEREGISTRAR_H
