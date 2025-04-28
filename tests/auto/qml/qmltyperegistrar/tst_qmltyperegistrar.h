// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TST_QMLTYPEREGISTRAR_H
#define TST_QMLTYPEREGISTRAR_H

#include "foreign.h"
#include "private/foreign_p.h"
#include "inaccessible/base.h"
#include "inaccessible/property.h"
#include "inaccessible/invisible.h"

#include <QtQmlTypeRegistrar/private/qqmltyperegistrar_p.h>

#ifdef QT_QUICK_LIB
#    include <QtQuick/qquickitem.h>
#endif

#include <QtQml/qqml.h>
#include <QtQml/qqmlcomponent.h>

#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qproperty.h>
#include <QtCore/qrect.h>
#include <QtCore/qtemporaryfile.h>
#include <QtCore/qtimeline.h>

class Interface1 {};
class Interface2 {};
class Interface3 {};

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(Interface1, "io.qt.bugreports.Interface1");
Q_DECLARE_INTERFACE(Interface2, "io.qt.bugreports.Interface2");
Q_DECLARE_INTERFACE(Interface3, "io.qt.bugreports.Interface3");
QT_END_NAMESPACE

class ImplementsInterfaces : public QObject, public Interface1
{
    Q_OBJECT
    QML_ELEMENT
    QML_IMPLEMENTS_INTERFACES(Interface1)
};

class ImplementsInterfaces2 : public QObject, public Interface1, public Interface2
{
    Q_OBJECT
    QML_ELEMENT
    QML_IMPLEMENTS_INTERFACES(Interface1 Interface2)
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

class AddedInLateMinorVersion : public QObject
{
    Q_OBJECT
    QML_ADDED_IN_VERSION(1, 5)
    Q_PROPERTY(int revisioned READ revisioned CONSTANT)
    QML_NAMED_ELEMENT(MinorVersioned)
public:
    AddedInLateMinorVersion(QObject *parent = nullptr) : QObject(parent) {}
    int revisioned() const { return 123; }
};

class RemovedInLateMinorVersion : public QObject
{
    Q_OBJECT
    QML_ADDED_IN_VERSION(1, 2)
    QML_REMOVED_IN_VERSION(1, 4)
    Q_PROPERTY(int revisioned READ revisioned CONSTANT)
    QML_NAMED_ELEMENT(MinorVersioned)
public:
    RemovedInLateMinorVersion(QObject *parent = nullptr) : QObject(parent) { }
    int revisioned() const { return 456; }
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

class AddedIn1_5 : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_ADDED_IN_VERSION(1, 5)
};

// Slightly absurd. The reason for such a thing may be a change in the versioning
// scheme of the base class. We still have to retain all of the version information
// so that you can at least use version 1.5.
class AddedIn1_0 : public AddedIn1_5
{
    Q_OBJECT
    QML_ELEMENT
    QML_ADDED_IN_VERSION(1, 0)
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

class Unconstructible
{
    Q_GADGET
    QML_VALUE_TYPE(unconstructible)
    int m_i = 11;
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

class Structured
{
    Q_GADGET
    QML_VALUE_TYPE(structured)
    QML_STRUCTURED_VALUE
    Q_PROPERTY(int i MEMBER m_i FINAL)

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
    enum UChar: uchar       { V0 = 41 };
    Q_ENUM(UChar)
    enum Int8_T: int8_t     { V1 = 42 };
    Q_ENUM(Int8_T)
    enum UInt8_T: uint8_t   { V2 = 43 };
    Q_ENUM(UInt8_T)
    enum Int16_T: int16_t   { V3 = 44 };
    Q_ENUM(Int16_T)
    enum UInt16_T: uint16_t { V4 = 45 };
    Q_ENUM(UInt16_T)
    enum Int32_T: int32_t   { V5 = 46 };
    Q_ENUM(Int32_T)
    enum UInt32_T: uint32_t { V6 = 47 };
    Q_ENUM(UInt32_T)

    // TODO: We cannot handle 64bit numbers as underlying types for enums.
    //       Luckily, moc generates bad code for those. So we don't have to, for now.

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

class NameExplosion : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Name1)
    QML_NAMED_ELEMENT(Name2)
    QML_ELEMENT
    QML_ANONYMOUS
};

class JavaScriptExtension : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("QML.Extended", "SymbolPrototype")
    Q_CLASSINFO("QML.ExtensionIsJavaScript", "true")
};

class LongNumberTypes : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(qint64   a MEMBER m_a)
    Q_PROPERTY(int64_t  b MEMBER m_b)
    Q_PROPERTY(quint64  c MEMBER m_c)
    Q_PROPERTY(uint64_t d MEMBER m_d)

    qint64   m_a = 1;
    int64_t  m_b = 2;
    quint64  m_c = 3;
    uint64_t m_d = 4;
};

struct EnumList
{
    Q_GADGET
    QML_ANONYMOUS
    QML_FOREIGN(QList<NetworkManager::NM>)
    QML_SEQUENTIAL_CONTAINER(NetworkManager::NM)
};

class ConstInvokable : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    Q_INVOKABLE const QObject *getObject() { return nullptr; }
};

using myint = int;

struct IntAlias
{
    Q_GADGET
    QML_FOREIGN(myint)
    QML_USING(int);
};

class WithMyInt : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(myint a READ a CONSTANT)
public:
    myint a() const { return 10; }
};

class UsesQtNamespace : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(Qt::Key key READ key CONSTANT)
public:
    Qt::Key key() const { return Qt::Key_Escape; }
};

class SlotsBeforeInvokables : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
public:
    Q_INVOKABLE void foo() {}
public Q_SLOTS:
    void bar() {}
public:
    Q_INVOKABLE void baz() {}
};

class JavaScriptFunction : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    JavaScriptFunction(QObject *parent = nullptr) : QObject(parent) {}
    Q_INVOKABLE void jsfunc(QQmlV4FunctionPtr) {}
};

struct UsingVoid {};
struct UsingVoidForeign
{
    Q_GADGET
    QML_FOREIGN(UsingVoid)
    QML_USING(void)
};

class VoidProperties : public QObject
{
    Q_OBJECT
    Q_PROPERTY(void* void1 READ void1 CONSTANT)
    Q_PROPERTY(UsingVoid* void2 READ void2 CONSTANT)
    QML_ELEMENT
public:
    VoidProperties(QObject *parent = nullptr) : QObject(parent) {}

    void *void1() const { return nullptr; }
    UsingVoid *void2() const { return nullptr; }
};

class AccessibleDerived : public InaccessibleBase
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(InaccessibleProperty *p MEMBER m_p CONSTANT)
public:
    AccessibleDerived(QObject *parent = nullptr) : InaccessibleBase(parent) {}
private:
    InaccessibleProperty *m_p = nullptr;
};

class EnumsExplicitlyScoped : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")
};

class DerivedFromInvisible : public InvisibleBase
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int b READ b CONSTANT)

public:
    int b() const { return m_b; }

private:
    int m_b = 7;
};

namespace F {
class ForeignQObject : public QObject {
    Q_OBJECT
public:
    enum class Enum{
        ValueA,
        ValueB
    };
    Q_ENUM(Enum)
};

class ForeignQOjbectQml
{
    Q_GADGET
    QML_FOREIGN(ForeignQObject)
    QML_NAMED_ELEMENT(ForeignQObject)
    QML_UNCREATABLE("can only be created in C++")
};
}

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
    void doNotRestrictToImportVersion();
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
    void singletonVersions();

#ifdef QT_QUICK_LIB
    void foreignRevisionedProperty();
#endif

    void addRemoveVersion_data();
    void addRemoveVersion();
    void addInMinorVersion();
    void typeInModuleMajorVersionZero();
    void resettableProperty();
    void duplicateExportWarnings();
    void clonedSignal();
    void baseVersionInQmltypes();
    void unconstructibleValueType();
    void constructibleValueType();
    void structuredValueType();
    void anonymousAndUncreatable();
    void omitInvisible();
    void typedEnum();
    void listSignal();
    void withNamespace();
    void sequenceRegistration();
    void valueTypeSelfReference();
    void foreignNamespaceFromGadget();

    void nameExplosion_data();
    void nameExplosion();

    void javaScriptExtension();

    void consistencyWarnings();
    void enumWarnings();

    void relatedAddedInVersion();
    void longNumberTypes();
    void enumList();
    void constReturnType();

    void usingDeclaration();
    void enumsRegistered();
    void doNotDuplicateQtNamespace();
    void doNotDuplicateQObject();
    void slotsBeforeInvokables();
    void allReferencedTypesCollected();

    void omitQQmlV4FunctionPtrArg();
    void preserveVoidStarPropTypes();

    void inaccessibleBase();
    void enumsExplicitlyScoped();
    void namespacedExtracted();
    void derivedFromInvisible();
    void foreignNamespacedWithEnum();

#ifdef QT_QMLJSROOTGEN_PRESENT
    void verifyJsRoot();
#endif

private:
    QByteArray qmltypesData;
    QString m_qmljsrootgenPath;
};

#endif // TST_QMLTYPEREGISTRAR_H
