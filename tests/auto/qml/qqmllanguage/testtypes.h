// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#ifndef TESTTYPES_H
#define TESTTYPES_H

#include <QtCore/qobject.h>
#include <QtCore/qrect.h>
#include <QtCore/qdatetime.h>
#include <QtCore/qjsonarray.h>
#include <QtGui/qtransform.h>
#include <QtGui/qcolor.h>
#include <QtGui/qvector2d.h>
#include <QtGui/qvector3d.h>
#include <QtGui/qvector4d.h>
#include <QtGui/qquaternion.h>
#include <QtQml/qqml.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlparserstatus.h>
#include <QtQml/qqmlpropertyvaluesource.h>
#include <QtQml/qqmlscriptstring.h>
#include <QtQml/qqmlproperty.h>

#include <private/qqmlcomponentattached_p.h>
#include <private/qqmlcustomparser_p.h>

QVariant myCustomVariantTypeConverter(const QString &data);

class MyInterface
{
public:
    MyInterface() : id(913) {}
    int id;
};

QT_BEGIN_NAMESPACE
#define MyInterface_iid "org.qt-project.Qt.Test.MyInterface"
Q_DECLARE_INTERFACE(MyInterface, MyInterface_iid);
QT_END_NAMESPACE
QML_DECLARE_INTERFACE(MyInterface);

struct MyCustomVariantType
{
    MyCustomVariantType() : a(0) {}
    int a;
};
Q_DECLARE_METATYPE(MyCustomVariantType);


class Group : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int value MEMBER value)

public:
    Group(QObject *parent = nullptr) : QObject(parent) {}
    int value = 0;
};


struct GroupGadget
{
    Q_GADGET

    Q_PROPERTY(int value MEMBER value)

public:
    friend bool operator==(GroupGadget g1, GroupGadget g2) { return g1.value == g2.value; }
    friend bool operator!=(GroupGadget g1, GroupGadget g2) { return !(g1 == g2); }
    int value = 0;
};

struct FakeDynamicObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString value MEMBER value)

public:
    FakeDynamicObject() {}
    QString value;
};

QT_BEGIN_NAMESPACE
namespace QtPrivate {
// don't do this at home – we override the meta-object which QMetaType collects for
// FakeDynamicObject* properties
template<>
struct MetaObjectForType<FakeDynamicObject *, void>
{
    static const QMetaObject *metaObjectFunction(const QMetaTypeInterface *);
};
}
QT_END_NAMESPACE

class UnexposedBase : public QObject
{
    Q_OBJECT

    Q_PROPERTY(Group *group MEMBER group)
    Q_PROPERTY(GroupGadget groupGadget MEMBER groupGadget)
    Q_PROPERTY(FakeDynamicObject *dynamic MEMBER dynamic)
public:
    UnexposedBase(QObject *parent = nullptr) : QObject(parent)
    {
        group = new Group(this);
    }
    Group *group;
    GroupGadget groupGadget;
    FakeDynamicObject *dynamic = nullptr;
};

class DerivedFromUnexposedBase : public UnexposedBase
{
    Q_OBJECT
    QML_ELEMENT
};


class MyAttachedObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(int value2 READ value2 WRITE setValue2)
    Q_PROPERTY(int length MEMBER length)
    Q_PROPERTY(QString name MEMBER name)
public:
    MyAttachedObject(QObject *parent) : QObject(parent), m_value(0), m_value2(0), name("attached") {}

    int value() const { return m_value; }
    void setValue(int v) { if (m_value != v) { m_value = v; emit valueChanged(); } }

    int value2() const { return m_value2; }
    void setValue2(int v) { m_value2 = v; }

signals:
    void valueChanged();

private:
    int m_value;
    int m_value2;
    int length = 10;
    QString name;
};

class SomethingUnknown : public QObject {
    Q_OBJECT
};

class SomethingKnown : public SomethingUnknown {
    Q_OBJECT
};

class MyQmlObject : public QObject, public MyInterface
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue FINAL)
    Q_PROPERTY(QString readOnlyString READ readOnlyString)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled)
    Q_PROPERTY(QRect rect READ rect WRITE setRect)
    Q_PROPERTY(QTransform transform READ transform WRITE setTransform) //assumed to be unsupported by QML
    Q_PROPERTY(MyInterface *interfaceProperty READ interface WRITE setInterface)
    Q_PROPERTY(int onLiteralSignal READ onLiteralSignal WRITE setOnLiteralSignal)
    Q_PROPERTY(MyCustomVariantType customType READ customType WRITE setCustomType)
    Q_PROPERTY(MyQmlObject *qmlobjectProperty READ qmlobject WRITE setQmlobject)
    Q_PROPERTY(int propertyWithNotify READ propertyWithNotify WRITE setPropertyWithNotify NOTIFY oddlyNamedNotifySignal)
    Q_PROPERTY(int nonScriptable READ nonScriptable WRITE setNonScriptable SCRIPTABLE false)
    Q_PROPERTY(QJSValue qjsvalue READ qjsvalue WRITE setQJSValue NOTIFY qjsvalueChanged)
    Q_PROPERTY(SomethingUnknown* somethingUnknown READ somethingUnknown WRITE setSomethingUnknown NOTIFY somethingUnknownChanged)

    Q_INTERFACES(MyInterface)
public:
    MyQmlObject();

    int value() const { return m_value; }
    void setValue(int v) { m_value = v; }

    QString readOnlyString() const { return QLatin1String(""); }

    bool enabled() const { return false; }
    void setEnabled(bool) {}

    QRect rect() const { return QRect(); }
    void setRect(const QRect&) {}

    QTransform transform() const { return QTransform(); }
    void setTransform(const QTransform &) {}

    MyInterface *interface() const { return m_interface; }
    void setInterface(MyInterface *iface) { m_interface = iface; }

    static MyAttachedObject *qmlAttachedProperties(QObject *other) {
        return new MyAttachedObject(other);
    }
    Q_CLASSINFO("DefaultMethod", "basicSlot()")

    int onLiteralSignal() const { return m_value; }
    void setOnLiteralSignal(int v) { m_value = v; }

    MyQmlObject *qmlobject() const { return m_qmlobject; }
    void setQmlobject(MyQmlObject *o) { m_qmlobject = o; }

    MyCustomVariantType customType() const { return m_custom; }
    void setCustomType(const MyCustomVariantType &v)  { m_custom = v; }

    int propertyWithNotify() const { return m_propertyWithNotify; }
    void setPropertyWithNotify(int i) { m_propertyWithNotify = i; emit oddlyNamedNotifySignal(); }

    int nonScriptable() const { return 0; }
    void setNonScriptable(int) {}

    QJSValue qjsvalue() const { return m_qjsvalue; }
    void setQJSValue(const QJSValue &value) { m_qjsvalue = value; emit qjsvalueChanged(); }

    int childAddedEventCount() const { return m_childAddedEventCount; }

    SomethingUnknown* somethingUnknown() const { return nullptr; }
    void setSomethingUnknown(SomethingUnknown* something) { Q_UNUSED(something); }

public slots:
    void basicSlot() { qWarning("MyQmlObject::basicSlot"); }
    void basicSlotWithArgs(int v) { qWarning("MyQmlObject::basicSlotWithArgs(%d)", v); }
    void qjsvalueMethod(const QJSValue &v) { m_qjsvalue = v; }

signals:
    void basicSignal();
    void basicParameterizedSignal(int parameter);
    void oddlyNamedNotifySignal();
    void signalWithDefaultArg(int parameter = 5);
    void qjsvalueChanged();
    void somethingUnknownChanged();

protected:
    bool event(QEvent *event) override;

private:
    friend class tst_qqmllanguage;
    int m_value;
    MyInterface *m_interface;
    MyQmlObject *m_qmlobject;
    MyCustomVariantType m_custom;
    int m_propertyWithNotify;
    QJSValue m_qjsvalue;
    int m_childAddedEventCount;
};
QML_DECLARE_TYPE(MyQmlObject)
QML_DECLARE_TYPEINFO(MyQmlObject, QML_HAS_ATTACHED_PROPERTIES)

class MyQmlObjectWithAttachedCounter : public QObject
{
    Q_OBJECT
public:
    MyQmlObjectWithAttachedCounter(QObject *parent = nullptr) : QObject(parent) { }
    static int attachedCount;

    static MyAttachedObject *qmlAttachedProperties(QObject *other)
    {
        ++MyQmlObjectWithAttachedCounter::attachedCount;
        return new MyAttachedObject(other);
    }
};
QML_DECLARE_TYPE(MyQmlObjectWithAttachedCounter)
QML_DECLARE_TYPEINFO(MyQmlObjectWithAttachedCounter, QML_HAS_ATTACHED_PROPERTIES)

class MyGroupedObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlScriptString script READ script WRITE setScript)
    Q_PROPERTY(int value READ value WRITE setValue)
public:
    QQmlScriptString script() const { return m_script; }
    void setScript(const QQmlScriptString &s) { m_script = s; }

    int value() const { return m_value; }
    void setValue(int v) { m_value = v; }

private:
    int m_value;
    QQmlScriptString m_script;
};


class MyEnumContainer : public QObject
{
    Q_OBJECT

public:
    enum RelatedEnum { RelatedInvalid = -1, RelatedValue = 42 };
    Q_ENUM(RelatedEnum)
};

class MyTypeObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString id READ id WRITE setId)
    Q_PROPERTY(QObject *objectProperty READ objectProperty WRITE setObjectProperty NOTIFY objectPropertyChanged)
    Q_PROPERTY(QQmlComponent *componentProperty READ componentProperty WRITE setComponentProperty)
    Q_PROPERTY(MyFlags flagProperty READ flagProperty WRITE setFlagProperty NOTIFY flagPropertyChanged)
    Q_PROPERTY(MyEnum enumProperty READ enumProperty WRITE setEnumProperty NOTIFY enumPropertyChanged)
    Q_PROPERTY(MyEnum readOnlyEnumProperty READ readOnlyEnumProperty)
    Q_PROPERTY(Qt::TextFormat qtEnumProperty READ qtEnumProperty WRITE setQtEnumProperty NOTIFY qtEnumPropertyChanged)
    Q_PROPERTY(MyMirroredEnum mirroredEnumProperty READ mirroredEnumProperty WRITE setMirroredEnumProperty NOTIFY mirroredEnumPropertyChanged)
    Q_PROPERTY(MyEnumContainer::RelatedEnum relatedEnumProperty READ relatedEnumProperty WRITE setRelatedEnumProperty)
    Q_PROPERTY(MyScopedEnum scopedEnum READ scopedEnum WRITE setScopedEnum)
    Q_PROPERTY(QString stringProperty READ stringProperty WRITE setStringProperty NOTIFY stringPropertyChanged)
    Q_PROPERTY(QByteArray byteArrayProperty READ byteArrayProperty WRITE setByteArrayProperty NOTIFY byteArrayPropertyChanged)
    Q_PROPERTY(uint uintProperty READ uintProperty WRITE setUintProperty NOTIFY uintPropertyChanged)
    Q_PROPERTY(int intProperty READ intProperty WRITE setIntProperty NOTIFY intPropertyChanged)
    Q_PROPERTY(qreal realProperty READ realProperty WRITE setRealProperty NOTIFY realPropertyChanged)
    Q_PROPERTY(double doubleProperty READ doubleProperty WRITE setDoubleProperty NOTIFY doublePropertyChanged)
    Q_PROPERTY(float floatProperty READ floatProperty WRITE setFloatProperty NOTIFY floatPropertyChanged)
    Q_PROPERTY(qsizetype qsizetypeProperty READ qsizetypeProperty WRITE setQsizetypeProperty NOTIFY qsizetypePropertyChanged)
    Q_PROPERTY(QColor colorProperty READ colorProperty WRITE setColorProperty NOTIFY colorPropertyChanged)
    Q_PROPERTY(QDate dateProperty READ dateProperty WRITE setDateProperty NOTIFY datePropertyChanged)
    Q_PROPERTY(QTime timeProperty READ timeProperty WRITE setTimeProperty NOTIFY timePropertyChanged)
    Q_PROPERTY(QDateTime dateTimeProperty READ dateTimeProperty WRITE setDateTimeProperty NOTIFY dateTimePropertyChanged)
    Q_PROPERTY(QPoint pointProperty READ pointProperty WRITE setPointProperty NOTIFY pointPropertyChanged)
    Q_PROPERTY(QPointF pointFProperty READ pointFProperty WRITE setPointFProperty NOTIFY pointFPropertyChanged)
    Q_PROPERTY(QSize sizeProperty READ sizeProperty WRITE setSizeProperty NOTIFY sizePropertyChanged)
    Q_PROPERTY(QSizeF sizeFProperty READ sizeFProperty WRITE setSizeFProperty NOTIFY sizeFPropertyChanged)
    Q_PROPERTY(QRect rectProperty READ rectProperty WRITE setRectProperty NOTIFY rectPropertyChanged)
    Q_PROPERTY(QRect rectProperty2 READ rectProperty2 WRITE setRectProperty2 )
    Q_PROPERTY(QRectF rectFProperty READ rectFProperty WRITE setRectFProperty NOTIFY rectFPropertyChanged)
    Q_PROPERTY(bool boolProperty READ boolProperty WRITE setBoolProperty NOTIFY boolPropertyChanged)
    Q_PROPERTY(QVariant variantProperty READ variantProperty WRITE setVariantProperty NOTIFY variantPropertyChanged)
    Q_PROPERTY(QVector2D vector2Property READ vector2Property WRITE setVector2Property NOTIFY vector2PropertyChanged)
    Q_PROPERTY(QVector3D vectorProperty READ vectorProperty WRITE setVectorProperty NOTIFY vectorPropertyChanged)
    Q_PROPERTY(QVector4D vector4Property READ vector4Property WRITE setVector4Property NOTIFY vector4PropertyChanged)
    Q_PROPERTY(QQuaternion quaternionProperty READ quaternionProperty WRITE setQuaternionProperty NOTIFY quaternionPropertyChanged)
    Q_PROPERTY(QUrl urlProperty READ urlProperty WRITE setUrlProperty NOTIFY urlPropertyChanged)

    Q_PROPERTY(QQmlScriptString scriptProperty READ scriptProperty WRITE setScriptProperty)
    Q_PROPERTY(MyGroupedObject *grouped READ grouped CONSTANT)
    Q_PROPERTY(MyGroupedObject *nullGrouped READ nullGrouped CONSTANT)

    Q_PROPERTY(MyTypeObject *selfGroupProperty READ selfGroupProperty)

public:
    MyTypeObject()
        : objectPropertyValue(0), componentPropertyValue(0) {}

    QString idValue;
    QString id() const {
        return idValue;
    }
    void setId(const QString &v) {
        idValue = v;
    }

    QObject *objectPropertyValue;
    QObject *objectProperty() const {
        return objectPropertyValue;
    }
    void setObjectProperty(QObject *v) {
        objectPropertyValue = v;
        emit objectPropertyChanged();
    }

    QQmlComponent *componentPropertyValue;
    QQmlComponent *componentProperty() const {
        return componentPropertyValue;
    }
    void setComponentProperty(QQmlComponent *v) {
        componentPropertyValue = v;
    }

    enum MyFlag { FlagVal1 = 0x01, FlagVal2 = 0x02, FlagVal3 = 0x04 };
    Q_DECLARE_FLAGS(MyFlags, MyFlag)
    Q_FLAG(MyFlags)
    MyFlags flagPropertyValue;
    MyFlags flagProperty() const {
        return flagPropertyValue;
    }
    void setFlagProperty(MyFlags v) {
        flagPropertyValue = v;
        emit flagPropertyChanged();
    }

    enum MyEnum { EnumVal1, EnumVal2, lowercaseEnumVal };
    Q_ENUM(MyEnum)

    MyEnum enumPropertyValue;
    MyEnum enumProperty() const {
        return enumPropertyValue;
    }
    void setEnumProperty(MyEnum v) {
        enumPropertyValue = v;
        emit enumPropertyChanged();
    }

    MyEnum readOnlyEnumProperty() const {
        return EnumVal1;
    }

    Qt::TextFormat qtEnumPropertyValue;
    Qt::TextFormat qtEnumProperty() const {
        return qtEnumPropertyValue;
    }
    void setQtEnumProperty(Qt::TextFormat v) {
        qtEnumPropertyValue = v;
        emit qtEnumPropertyChanged();
    }

    enum MyMirroredEnum {
        MirroredEnumVal1 = Qt::AlignLeft,
        MirroredEnumVal2 = Qt::AlignRight,
        MirroredEnumVal3 = Qt::AlignHCenter };
    Q_ENUM(MyMirroredEnum)

    MyMirroredEnum mirroredEnumPropertyValue;
    MyMirroredEnum mirroredEnumProperty() const {
        return mirroredEnumPropertyValue;
    }
    void setMirroredEnumProperty(MyMirroredEnum v) {
        mirroredEnumPropertyValue = v;
        emit mirroredEnumPropertyChanged();
    }

    MyEnumContainer::RelatedEnum relatedEnumPropertyValue;
    MyEnumContainer::RelatedEnum relatedEnumProperty() const {
        return relatedEnumPropertyValue;
    }
    void setRelatedEnumProperty(MyEnumContainer::RelatedEnum v) {
        relatedEnumPropertyValue = v;
    }

    enum class MyScopedEnum : int { ScopedVal1, ScopedVal2, ScopedVal3 };
    Q_ENUM(MyScopedEnum)
    MyScopedEnum scopedEnumPropertyValue;
    MyScopedEnum scopedEnum() const { return scopedEnumPropertyValue; }
    void setScopedEnum(MyScopedEnum v) {
        scopedEnumPropertyValue = v;
    }

    QString stringPropertyValue;
    QString stringProperty() const {
       return stringPropertyValue;
    }
    void setStringProperty(const QString &v) {
        stringPropertyValue = v;
        emit stringPropertyChanged();
    }

    QByteArray byteArrayPropertyValue;
    QByteArray byteArrayProperty() const {
        return byteArrayPropertyValue;
    }
    void setByteArrayProperty(const QByteArray &v) {
        byteArrayPropertyValue = v;
        emit byteArrayPropertyChanged();
    }

    uint uintPropertyValue;
    uint uintProperty() const {
       return uintPropertyValue;
    }
    void setUintProperty(const uint &v) {
        uintPropertyValue = v;
        emit uintPropertyChanged();
    }

    int intPropertyValue;
    int intProperty() const {
       return intPropertyValue;
    }
    void setIntProperty(const int &v) {
        intPropertyValue = v;
        emit intPropertyChanged();
    }

    qsizetype qsizetypePropertyValue;
    qsizetype qsizetypeProperty() const {
        return qsizetypePropertyValue;
    }
    void setQsizetypeProperty(const qsizetype &v) {
        qsizetypePropertyValue = v;
        emit qsizetypePropertyChanged();
    }

    qreal realPropertyValue;
    qreal realProperty() const {
       return realPropertyValue;
    }
    void setRealProperty(const qreal &v) {
        realPropertyValue = v;
        emit realPropertyChanged();
    }

    double doublePropertyValue;
    double doubleProperty() const {
       return doublePropertyValue;
    }
    void setDoubleProperty(const double &v) {
        doublePropertyValue = v;
        emit doublePropertyChanged();
    }

    float floatPropertyValue;
    float floatProperty() const {
       return floatPropertyValue;
    }
    void setFloatProperty(const float &v) {
        floatPropertyValue = v;
        emit floatPropertyChanged();
    }

    QColor colorPropertyValue;
    QColor colorProperty() const {
       return colorPropertyValue;
    }
    void setColorProperty(const QColor &v) {
        colorPropertyValue = v;
        emit colorPropertyChanged();
    }

    QDate datePropertyValue;
    QDate dateProperty() const {
       return datePropertyValue;
    }
    void setDateProperty(QDate v) {
        datePropertyValue = v;
        emit datePropertyChanged();
    }

    QTime timePropertyValue;
    QTime timeProperty() const {
       return timePropertyValue;
    }
    void setTimeProperty(QTime v) {
        timePropertyValue = v;
        emit timePropertyChanged();
    }

    QDateTime dateTimePropertyValue;
    QDateTime dateTimeProperty() const {
       return dateTimePropertyValue;
    }
    void setDateTimeProperty(const QDateTime &v) {
        dateTimePropertyValue = v;
        emit dateTimePropertyChanged();
    }

    QPoint pointPropertyValue;
    QPoint pointProperty() const {
       return pointPropertyValue;
    }
    void setPointProperty(const QPoint &v) {
        pointPropertyValue = v;
        emit pointPropertyChanged();
    }

    QPointF pointFPropertyValue;
    QPointF pointFProperty() const {
       return pointFPropertyValue;
    }
    void setPointFProperty(const QPointF &v) {
        pointFPropertyValue = v;
        emit pointFPropertyChanged();
    }

    QSize sizePropertyValue;
    QSize sizeProperty() const {
       return sizePropertyValue;
    }
    void setSizeProperty(const QSize &v) {
        sizePropertyValue = v;
        emit sizePropertyChanged();
    }

    QSizeF sizeFPropertyValue;
    QSizeF sizeFProperty() const {
       return sizeFPropertyValue;
    }
    void setSizeFProperty(const QSizeF &v) {
        sizeFPropertyValue = v;
        emit sizeFPropertyChanged();
    }

    QRect rectPropertyValue;
    QRect rectProperty() const {
       return rectPropertyValue;
    }
    void setRectProperty(const QRect &v) {
        rectPropertyValue = v;
        emit rectPropertyChanged();
    }

    QRect rectPropertyValue2;
    QRect rectProperty2() const {
       return rectPropertyValue2;
    }
    void setRectProperty2(const QRect &v) {
        rectPropertyValue2 = v;
    }

    QRectF rectFPropertyValue;
    QRectF rectFProperty() const {
       return rectFPropertyValue;
    }
    void setRectFProperty(const QRectF &v) {
        rectFPropertyValue = v;
        emit rectFPropertyChanged();
    }

    bool boolPropertyValue;
    bool boolProperty() const {
       return boolPropertyValue;
    }
    void setBoolProperty(const bool &v) {
        boolPropertyValue = v;
        emit boolPropertyChanged();
    }

    QVariant variantPropertyValue;
    QVariant variantProperty() const {
       return variantPropertyValue;
    }
    void setVariantProperty(const QVariant &v) {
        variantPropertyValue = v;
        emit variantPropertyChanged();
    }

    QVector3D vectorPropertyValue;
    QVector3D vectorProperty() const {
        return vectorPropertyValue;
    }
    void setVectorProperty(const QVector3D &v) {
        vectorPropertyValue = v;
        emit vectorPropertyChanged();
    }

    QVector2D vector2PropertyValue;
    QVector2D vector2Property() const {
        return vector2PropertyValue;
    }
    void setVector2Property(const QVector2D &v) {
        vector2PropertyValue = v;
        emit vector2PropertyChanged();
    }

    QVector4D vector4PropertyValue;
    QVector4D vector4Property() const {
        return vector4PropertyValue;
    }
    void setVector4Property(const QVector4D &v) {
        vector4PropertyValue = v;
        emit vector4PropertyChanged();
    }

    QQuaternion quaternionPropertyValue;
    QQuaternion quaternionProperty() const {
        return quaternionPropertyValue;
    }
    void setQuaternionProperty(const QQuaternion &v) {
        quaternionPropertyValue = v;
        emit quaternionPropertyChanged();
    }

    QUrl urlPropertyValue;
    QUrl urlProperty() const {
        return urlPropertyValue;
    }
    void setUrlProperty(const QUrl &v) {
        urlPropertyValue = v;
        emit urlPropertyChanged();
    }

    QQmlScriptString scriptPropertyValue;
    QQmlScriptString scriptProperty() const {
        return scriptPropertyValue;
    }
    void setScriptProperty(const QQmlScriptString &v) {
        scriptPropertyValue = v;
    }

    MyGroupedObject groupedValue;
    MyGroupedObject *grouped() { return &groupedValue; }

    MyGroupedObject *nullGrouped() { return 0; }

    MyTypeObject *selfGroupProperty() { return this; }

    void doAction() { emit action(); }
signals:
    void action();

    void objectPropertyChanged();
    void flagPropertyChanged();
    void enumPropertyChanged();
    void qtEnumPropertyChanged();
    void mirroredEnumPropertyChanged();
    void stringPropertyChanged();
    void byteArrayPropertyChanged();
    void uintPropertyChanged();
    void intPropertyChanged();
    void qsizetypePropertyChanged();
    void realPropertyChanged();
    void doublePropertyChanged();
    void floatPropertyChanged();
    void colorPropertyChanged();
    void datePropertyChanged();
    void timePropertyChanged();
    void dateTimePropertyChanged();
    void pointPropertyChanged();
    void pointFPropertyChanged();
    void sizePropertyChanged();
    void sizeFPropertyChanged();
    void rectPropertyChanged();
    void rectFPropertyChanged();
    void boolPropertyChanged();
    void variantPropertyChanged();
    void vectorPropertyChanged();
    void vector2PropertyChanged();
    void vector4PropertyChanged();
    void quaternionPropertyChanged();
    void urlPropertyChanged();

};
Q_DECLARE_OPERATORS_FOR_FLAGS(MyTypeObject::MyFlags)

// FIXME: If no subclass is used for the singleton registration with qmlRegisterSingletonType(),
//        the valueTypes() test will fail.
class MyTypeObjectSingleton : public MyTypeObject
{
    Q_OBJECT
};

class MyContainer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<QObject> children READ children)
    Q_PROPERTY(QQmlListProperty<MyContainer> containerChildren READ containerChildren)
    Q_PROPERTY(QQmlListProperty<MyInterface> qlistInterfaces READ qlistInterfaces)
    Q_CLASSINFO("DefaultProperty", "children")
public:
    MyContainer() {}

    QQmlListProperty<QObject> children() { return QQmlListProperty<QObject>(this, &m_children); }
    QQmlListProperty<MyContainer> containerChildren() { return QQmlListProperty<MyContainer>(this, &m_containerChildren); }
    QList<QObject *> *getChildren() { return &m_children; }
    QQmlListProperty<MyInterface> qlistInterfaces() { return QQmlListProperty<MyInterface>(this, &m_interfaces); }
    QList<MyInterface *> *getQListInterfaces() { return &m_interfaces; }

    QList<MyContainer*> m_containerChildren;
    QList<QObject*> m_children;
    QList<MyInterface *> m_interfaces;
};


class MyPropertyValueSource : public QObject, public QQmlPropertyValueSource
{
    Q_OBJECT
    Q_INTERFACES(QQmlPropertyValueSource)
public:
    MyPropertyValueSource()
        : QQmlPropertyValueSource() {}

    QQmlProperty prop;
    void setTarget(const QQmlProperty &p) override
    {
        prop = p;
    }
};

class UnavailableType : public QObject
{
    Q_OBJECT
public:
    UnavailableType() {}
};

class MyReceiversTestObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int prop READ prop NOTIFY propChanged)
public:
    MyReceiversTestObject()  {}

    int prop() const { return 5; }

    int mySignalCount() { return receivers(SIGNAL(mySignal())); }
    int propChangedCount() { return receivers(SIGNAL(propChanged())); }
    int myUnconnectedSignalCount() { return receivers(SIGNAL(myUnconnectedSignal())); }

signals:
    void mySignal();
    void propChanged();
    void myUnconnectedSignal();

private:
    friend class tst_qqmllanguage;
};

class MyDotPropertyObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(MyQmlObject *obj READ obj)
    Q_PROPERTY(MyQmlObject *readWriteObj READ readWriteObj WRITE setReadWriteObj)
public:
    MyDotPropertyObject() : m_rwobj(0), m_ownRWObj(false) {}
    ~MyDotPropertyObject()
    {
        if (m_ownRWObj)
            delete m_rwobj;
    }

    MyQmlObject *obj() { return 0; }

    MyQmlObject *readWriteObj()
    {
        if (!m_rwobj) {
            m_rwobj = new MyQmlObject;
            m_ownRWObj = true;
        }
        return m_rwobj;
    }

    void setReadWriteObj(MyQmlObject *obj)
    {
        if (m_ownRWObj) {
            delete m_rwobj;
            m_ownRWObj = false;
        }

        m_rwobj = obj;
    }

private:
    MyQmlObject *m_rwobj;
    bool m_ownRWObj;
};

namespace MyStaticNamespace {
    Q_NAMESPACE
    QML_ELEMENT

    enum MyNSEnum {
        Key1 = 1,
        Key2,
        Key5 = 5
    };
    Q_ENUM_NS(MyNSEnum);

    enum class MyOtherNSEnum {
        OtherKey1 = 1,
        OtherKey2
    };
    Q_ENUM_NS(MyOtherNSEnum);


    class MyNamespacedType : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(MyStaticNamespace::MyNSEnum myEnum MEMBER m_myEnum)
        QML_NAMED_ELEMENT(MyStaticNamespacedType)
        MyStaticNamespace::MyNSEnum m_myEnum = MyNSEnum::Key1;
    };

    class MySecondNamespacedType : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QQmlListProperty<MyStaticNamespace::MyNamespacedType> list READ list)
        QML_NAMED_ELEMENT(MyStaticSecondNamespacedType)
    public:
        QQmlListProperty<MyNamespacedType> list()
        {
            return QQmlListProperty<MyNamespacedType>(this, &m_list);
        }

    private:
        QList<MyNamespacedType *> m_list;
    };
}

namespace MyNamespace {
    Q_NAMESPACE
    enum MyNSEnum {
        Key1 = 1,
        Key2,
        Key5 = 5
    };
    Q_ENUM_NS(MyNSEnum);

    enum class MyOtherNSEnum {
        OtherKey1 = 1,
        OtherKey2
    };
    Q_ENUM_NS(MyOtherNSEnum);


    class MyNamespacedType : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(MyNamespace::MyNSEnum myEnum MEMBER m_myEnum)
        MyNamespace::MyNSEnum m_myEnum = MyNSEnum::Key1;
    };

    class MySecondNamespacedType : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(QQmlListProperty<MyNamespace::MyNamespacedType> list READ list)
    public:
        QQmlListProperty<MyNamespacedType> list() { return QQmlListProperty<MyNamespacedType>(this, &m_list); }

    private:
        QList<MyNamespacedType *> m_list;
    };
}

class MyCustomParserType : public QObject
{
    Q_OBJECT
};

class MyCustomParserTypeParser : public QQmlCustomParser
{
public:
    void verifyBindings(
            const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &,
            const QList<const QV4::CompiledData::Binding *> &) override {}
    void applyBindings(
            QObject *, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &,
            const QList<const QV4::CompiledData::Binding *> &) override {}
};

class EnumSupportingCustomParser : public QQmlCustomParser
{
public:
    void verifyBindings(
            const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &,
            const QList<const QV4::CompiledData::Binding *> &) override;
    void applyBindings(
            QObject *, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &,
            const QList<const QV4::CompiledData::Binding *> &) override {}
};

class MyParserStatus : public QObject, public QQmlParserStatus
{
    Q_INTERFACES(QQmlParserStatus)
    Q_OBJECT
public:
    MyParserStatus() : m_cbc(0), m_ccc(0) {}

    int classBeginCount() const { return m_cbc; }
    int componentCompleteCount() const { return m_ccc; }

    void classBegin() override { m_cbc++; }
    void componentComplete() override { m_ccc++; }
private:
    int m_cbc;
    int m_ccc;
};

class MyRevisionedBaseClassRegistered : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal propA READ propA WRITE setPropA NOTIFY propAChanged)
    Q_PROPERTY(qreal propB READ propB WRITE setPropB NOTIFY propBChanged REVISION 1)

public:
    MyRevisionedBaseClassRegistered() : m_pa(1), m_pb(2) {}

    qreal propA() const { return m_pa; }
    void setPropA(qreal p) {
        if (p != m_pa) {
            m_pa = p;
            emit propAChanged();
        }
    }
    qreal propB() const { return m_pb; }
    void setPropB(qreal p) {
        if (p != m_pb) {
            m_pb = p;
            emit propBChanged();
        }
    }

    Q_INVOKABLE void methodA() { }
    Q_INVOKABLE Q_REVISION(1) void methodB() { }

signals:
    void propAChanged();
    void propBChanged();

    void signalA();
    Q_REVISION(1) void signalB();

protected:
    qreal m_pa;
    qreal m_pb;
};

class MyRevisionedIllegalOverload : public MyRevisionedBaseClassRegistered
{
    Q_OBJECT
    Q_PROPERTY(qreal propA READ propA WRITE setPropA REVISION 1);
};

class MyRevisionedLegalOverload : public MyRevisionedBaseClassRegistered
{
    Q_OBJECT
    Q_PROPERTY(qreal propB READ propB WRITE setPropB REVISION 1);
};

class MyRevisionedBaseClassUnregistered : public MyRevisionedBaseClassRegistered
{
    Q_OBJECT
    Q_PROPERTY(qreal propC READ propC WRITE setPropC NOTIFY propCChanged)
    Q_PROPERTY(qreal propD READ propD WRITE setPropD NOTIFY propDChanged REVISION 1)

public:
    MyRevisionedBaseClassUnregistered() : m_pc(1), m_pd(2) {}

    qreal propC() const { return m_pc; }
    void setPropC(qreal p) {
        if (p != m_pc) {
            m_pc = p;
            emit propCChanged();
        }
    }
    qreal propD() const { return m_pd; }
    void setPropD(qreal p) {
        if (p != m_pd) {
            m_pd = p;
            emit propDChanged();
        }
    }

    Q_INVOKABLE void methodC() { }
    Q_INVOKABLE Q_REVISION(1) void methodD() { }

signals:
    void propCChanged();
    void propDChanged();

    void signalC();
    Q_REVISION(1) void signalD();

protected:
    qreal m_pc;
    qreal m_pd;
};

class MyRevisionedClass : public MyRevisionedBaseClassUnregistered
{
    Q_OBJECT
    Q_PROPERTY(qreal prop1 READ prop1 WRITE setProp1 NOTIFY prop1Changed)
    Q_PROPERTY(qreal prop2 READ prop2 WRITE setProp2 NOTIFY prop2Changed REVISION 1)

public:
    MyRevisionedClass() : m_p1(1), m_p2(2) {}

    qreal prop1() const { return m_p1; }
    void setProp1(qreal p) {
        if (p != m_p1) {
            m_p1 = p;
            emit prop1Changed();
        }
    }
    qreal prop2() const { return m_p2; }
    void setProp2(qreal p) {
        if (p != m_p2) {
            m_p2 = p;
            emit prop2Changed();
        }
    }

    Q_INVOKABLE void method1() { }
    Q_INVOKABLE Q_REVISION(1) void method2() { }

signals:
    void prop1Changed();
    void prop2Changed();

    void signal1();
    Q_REVISION(1) void signal2();

protected:
    qreal m_p1;
    qreal m_p2;
};

class MyRevisionedSubclass : public MyRevisionedClass
{
    Q_OBJECT
    Q_PROPERTY(qreal prop3 READ prop3 WRITE setProp3 NOTIFY prop3Changed)
    Q_PROPERTY(qreal prop4 READ prop4 WRITE setProp4 NOTIFY prop4Changed REVISION 1)

public:
    MyRevisionedSubclass() : m_p3(3), m_p4(4) {}

    qreal prop3() const { return m_p3; }
    void setProp3(qreal p) {
        if (p != m_p3) {
            m_p3 = p;
            emit prop3Changed();
        }
    }
    qreal prop4() const { return m_p4; }
    void setProp4(qreal p) {
        if (p != m_p4) {
            m_p4 = p;
            emit prop4Changed();
        }
    }

    Q_INVOKABLE void method3() { }
    Q_INVOKABLE Q_REVISION(1) void method4() { }

signals:
    void prop3Changed();
    void prop4Changed();

    void signal3();
    Q_REVISION(1) void signal4();

protected:
    qreal m_p3;
    qreal m_p4;
};

class MySubclass : public MyRevisionedClass
{
    Q_OBJECT
    Q_PROPERTY(qreal prop5 READ prop5 WRITE setProp5 NOTIFY prop5Changed)

public:
    MySubclass() : m_p5(5) {}

    qreal prop5() const { return m_p5; }
    void setProp5(qreal p) {
        if (p != m_p5) {
            m_p5 = p;
            emit prop5Changed();
        }
    }

    Q_INVOKABLE void method5() { }

signals:
    void prop5Changed();

protected:
    qreal m_p5;
};

class MyUncreateableBaseClass : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool prop1 READ prop1 WRITE setprop1)
    Q_PROPERTY(bool prop2 READ prop2 WRITE setprop2 REVISION 1)
    Q_PROPERTY(bool prop3 READ prop3 WRITE setprop3 REVISION 1)
public:
    explicit MyUncreateableBaseClass(bool /* arg */, QObject *parent = nullptr)
        : QObject(parent), _prop1(false), _prop2(false), _prop3(false)
    {
    }

    bool _prop1;
    bool prop1() const { return _prop1; }
    void setprop1(bool p) { _prop1 = p; }
    bool _prop2;
    bool prop2() const { return _prop2; }
    void setprop2(bool p) { _prop2 = p; }
    bool _prop3;
    bool prop3() const { return _prop3; }
    void setprop3(bool p) { _prop3 = p; }
};

class MyCreateableDerivedClass : public MyUncreateableBaseClass
{
    Q_OBJECT
    Q_PROPERTY(bool prop2 READ prop2 WRITE setprop2 REVISION 1)

public:
    MyCreateableDerivedClass(QObject *parent = nullptr)
        : MyUncreateableBaseClass(true, parent)
    {
    }
};

class MyExtendedUncreateableBaseClass : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool prop1 READ prop1 WRITE setprop1)
    Q_PROPERTY(bool prop2 READ prop2 WRITE setprop2 REVISION 1)
    Q_PROPERTY(bool prop3 READ prop3 WRITE setprop3 REVISION 1)
public:
    explicit MyExtendedUncreateableBaseClass(QObject *parent = nullptr)
        : QObject(parent), _prop1(false), _prop2(false), _prop3(false)
    {
    }

    bool _prop1;
    bool prop1() const { return _prop1; }
    void setprop1(bool p) { _prop1 = p; }
    bool _prop2;
    bool prop2() const { return _prop2; }
    void setprop2(bool p) { _prop2 = p; }
    bool _prop3;
    bool prop3() const { return _prop3; }
    void setprop3(bool p) { _prop3 = p; }
};

class MyExtendedUncreateableBaseClassExtension : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool prop4 READ prop4 WRITE setprop4)
public:
    explicit MyExtendedUncreateableBaseClassExtension(QObject *parent = nullptr)
        : QObject(parent), _prop4(false)
    {
    }

    bool _prop4;
    bool prop4() const { return _prop4; }
    void setprop4(bool p) { _prop4 = p; }
};

class MyExtendedCreateableDerivedClass : public MyExtendedUncreateableBaseClass
{
    Q_OBJECT
    Q_PROPERTY(bool prop5 READ prop5 WRITE setprop5)

public:
    MyExtendedCreateableDerivedClass(QObject *parent = nullptr)
        : MyExtendedUncreateableBaseClass(parent), _prop5(false)
    {
    }

    bool _prop5;
    bool prop5() const { return _prop5; }
    void setprop5(bool p) { _prop5 = p; }
};

class MyVersion2Class : public QObject
{
    Q_OBJECT
};

class MyEnum1Class : public QObject
{
    Q_OBJECT

public:
    MyEnum1Class() : value(A_Invalid) {}

    enum EnumA
    {
        A_Invalid = -1,

        A_11 = 11,
        A_13 = 13
    };
    Q_ENUM(EnumA)

    Q_INVOKABLE void setValue(EnumA v) { value = v; }

    EnumA getValue() { return value; }

private:
    EnumA value;
};

class MyEnum2Class : public QObject
{
    Q_OBJECT

public:
    MyEnum2Class() : valueA(MyEnum1Class::A_Invalid), valueB(B_Invalid), valueC(Qt::PlainText),
                     valueD(Qt::ElideLeft), valueE(E_Invalid), valueE2(E_Invalid) {}

    enum EnumB
    {
        B_Invalid = -1,

        B_29 = 29,
        B_31 = 31,
        B_37 = 37
    };
    Q_ENUM(EnumB)

    enum EnumE
    {
        E_Invalid = -1,

        E_14 = 14,
        E_76 = 76
    };
    Q_ENUM(EnumE)

    MyEnum1Class::EnumA getValueA() { return valueA; }
    EnumB getValueB() { return valueB; }
    Qt::TextFormat getValueC() { return valueC; }
    Qt::TextElideMode getValueD() { return valueD; }
    EnumE getValueE() { return valueE; }
    EnumE getValueE2() { return valueE2; }

    Q_INVOKABLE void setValueA(MyEnum1Class::EnumA v) { valueA = v; emit valueAChanged(v); }
    Q_INVOKABLE void setValueB(EnumB v) { valueB = v; emit valueBChanged(v); }
    Q_INVOKABLE void setValueC(Qt::TextFormat v) { valueC = v; emit valueCChanged(v); }     //registered
    Q_INVOKABLE void setValueD(Qt::TextElideMode v) { valueD = v; emit valueDChanged(v); }  //unregistered
    Q_INVOKABLE void setValueE(EnumE v) { valueE = v; emit valueEChanged(v); }
    Q_INVOKABLE void setValueE2(MyEnum2Class::EnumE v) { valueE2 = v; emit valueE2Changed(v); }

signals:
    void valueAChanged(MyEnum1Class::EnumA newValue);
    void valueBChanged(MyEnum2Class::EnumB newValue);
    void valueCChanged(Qt::TextFormat newValue);
    void valueDChanged(Qt::TextElideMode newValue);
    void valueEChanged(EnumE newValue);
    void valueE2Changed(MyEnum2Class::EnumE newValue);

private:
    MyEnum1Class::EnumA valueA;
    EnumB valueB;
    Qt::TextFormat valueC;
    Qt::TextElideMode valueD;
    EnumE valueE;
    EnumE valueE2;
};

class MyEnumDerivedClass : public MyEnum2Class
{
    Q_OBJECT
};

class MyCompositeBaseType : public QObject
{
    Q_OBJECT

public:
    enum CompositeEnum { EnumValue0, EnumValue42 = 42 };
    Q_ENUM(CompositeEnum)

    enum class ScopedCompositeEnum : int { EnumValue15 = 15 };
    Q_ENUM(ScopedCompositeEnum)

    static QObject *qmlAttachedProperties(QObject *parent) { return new QObject(parent); }
};

class MyArrayBufferTestClass : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QByteArray byteArrayProperty READ byteArrayProperty WRITE setByteArrayProperty NOTIFY byteArrayPropertyChanged)

signals:
    void byteArrayPropertyChanged();
    void byteArraySignal(QByteArray arg);

public:
    QByteArray byteArrayPropertyValue;
    QByteArray byteArrayProperty() const {
        return byteArrayPropertyValue;
    }
    void setByteArrayProperty(const QByteArray &v) {
        byteArrayPropertyValue = v;
        emit byteArrayPropertyChanged();
    }
    Q_INVOKABLE void emitByteArraySignal(char begin, char num) {
        byteArraySignal(byteArrayMethod_CountUp(begin, num));
    }
    Q_INVOKABLE int byteArrayMethod_Sum(QByteArray arg) {
        int sum = 0;
        for (int i = 0; i < arg.size(); ++i) {
            sum += arg[i];
        }
        return sum;
    }
    Q_INVOKABLE QByteArray byteArrayMethod_CountUp(char begin, int num) {
        QByteArray ret;
        for (int i = 0; i < num; ++i) {
            ret.push_back(begin++);
        }
        return ret;
    }
    Q_INVOKABLE bool byteArrayMethod_Overloaded(QByteArray) {
        return true;
    }
    Q_INVOKABLE bool byteArrayMethod_Overloaded(int) {
        return false;
    }
    Q_INVOKABLE bool byteArrayMethod_Overloaded(QString) {
        return false;
    }
    Q_INVOKABLE bool byteArrayMethod_Overloaded(QJSValue) {
        return false;
    }
    Q_INVOKABLE bool byteArrayMethod_Overloaded(QVariant) {
        return false;
    }
};

class EnumPropsManyUnderlyingTypes : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    enum  si8 : qint8 { ResolvedValue = 1};
    enum  ui8 : quint8 {};
    enum si16 : qint16 {};
    enum ui16 : quint16 {};
    enum si64 : qint64 {};
    enum ui64 : quint64 {};
    Q_ENUM(si8)
    Q_ENUM(ui8)
    Q_ENUM(si16)
    Q_ENUM(ui16)
    Q_ENUM(si64)
    Q_ENUM(ui64)


    Q_PROPERTY(si8  si8prop MEMBER si8prop)
    Q_PROPERTY(ui8  ui8prop MEMBER ui8prop)
    Q_PROPERTY(si16 si16prop MEMBER si16prop)
    Q_PROPERTY(ui16 ui16prop MEMBER ui16prop)
    Q_PROPERTY(si64 si64prop MEMBER si64prop)
    Q_PROPERTY(ui64 ui64prop MEMBER ui64prop)

    si8 si8prop = si8(0);
    ui8 ui8prop = ui8(0);
    si16 si16prop = si16(0);
    ui16 ui16prop = ui16(0);
    si64 si64prop = si64(0);
    ui64 ui64prop = ui64(0);
};

Q_DECLARE_METATYPE(MyEnum2Class::EnumB)
Q_DECLARE_METATYPE(MyEnum1Class::EnumA)
Q_DECLARE_METATYPE(Qt::TextFormat)
Q_DECLARE_METATYPE(MyCompositeBaseType::CompositeEnum)

QML_DECLARE_TYPE(MyRevisionedBaseClassRegistered)
QML_DECLARE_TYPE(MyRevisionedBaseClassUnregistered)
QML_DECLARE_TYPE(MyRevisionedClass)
QML_DECLARE_TYPE(MyRevisionedSubclass)
QML_DECLARE_TYPE(MySubclass)
QML_DECLARE_TYPE(MyReceiversTestObject)
QML_DECLARE_TYPE(MyCompositeBaseType)
QML_DECLARE_TYPEINFO(MyCompositeBaseType, QML_HAS_ATTACHED_PROPERTIES)

class CustomBinding : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)
    Q_PROPERTY(QObject* target READ target WRITE setTarget)
public:

    void classBegin() override {}
    void componentComplete() override;

    QObject *target() const { return m_target; }
    void setTarget(QObject *newTarget) { m_target = newTarget; }

    QPointer<QObject> m_target;
    QQmlRefPointer<QV4::ExecutableCompilationUnit> compilationUnit;
    QList<const QV4::CompiledData::Binding*> bindings;
    QByteArray m_bindingData;
};

class CustomBindingParser : public QQmlCustomParser
{
    void verifyBindings(
            const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &,
            const QList<const QV4::CompiledData::Binding *> &) override {}
    void applyBindings(
            QObject *, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &,
            const QList<const QV4::CompiledData::Binding *> &) override;
};

class SimpleObjectWithCustomParser : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int intProperty READ intProperty WRITE setIntProperty)
public:
    SimpleObjectWithCustomParser()
        : m_intProperty(0)
        , m_customBindingsCount(0)
    {}

    int intProperty() const { return m_intProperty; }
    void setIntProperty(int value) { m_intProperty = value; }

    void setCustomBindingsCount(int count) { m_customBindingsCount = count; }
    int customBindingsCount() const { return m_customBindingsCount; }

private:
    int m_intProperty;
    int m_customBindingsCount;
};

class SimpleObjectExtension : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int extendedProperty READ extendedProperty WRITE setExtendedProperty NOTIFY extendedPropertyChanged)
public:
    SimpleObjectExtension(QObject *parent = nullptr)
        : QObject(parent)
        , m_extendedProperty(1584)
    {}

    void setExtendedProperty(int extendedProperty) { m_extendedProperty = extendedProperty; emit extendedPropertyChanged(); }
    int extendedProperty() const { return m_extendedProperty; }

signals:
   void extendedPropertyChanged();
private:
   int m_extendedProperty;
};

class SimpleObjectCustomParser : public QQmlCustomParser
{
    void verifyBindings(
            const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &,
            const QList<const QV4::CompiledData::Binding *> &) override {}
    void applyBindings(
            QObject *, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &,
            const QList<const QV4::CompiledData::Binding *> &) override;
};

class RootObjectInCreationTester : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject *subObject READ subObject WRITE setSubObject FINAL)
    Q_CLASSINFO("DeferredPropertyNames", "subObject");
public:
    RootObjectInCreationTester()
        : obj(0)
    {}

    QObject *subObject() const { return obj; }
    void setSubObject(QObject *o) { obj = o; }

private:
    QObject *obj;
};

class LazyDeferredSubObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject *subObject READ subObject WRITE setSubObject NOTIFY subObjectChanged FINAL)
    Q_CLASSINFO("DeferredPropertyNames", "subObject");
public:
    LazyDeferredSubObject()
        : obj(0)
    {}

    QObject *subObject() const { if (!obj) qmlExecuteDeferred(const_cast<LazyDeferredSubObject *>(this)); return obj; }
    void setSubObject(QObject *o) { if (obj == o) return; obj = o; emit subObjectChanged(); }

signals:
    void subObjectChanged();

private:
    QObject *obj;
};

class DeferredProperties : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QObject *groupProperty MEMBER m_group)
    Q_PROPERTY(QQmlListProperty<QObject> listProperty READ listProperty)
    Q_CLASSINFO("DeferredPropertyNames", "groupProperty,listProperty")
    Q_CLASSINFO("DefaultProperty", "listProperty")
public:
    QQmlListProperty<QObject> listProperty() { return QQmlListProperty<QObject>(this, &m_list); }

private:
    QObject *m_group = 0;
    QObjectList m_list;
};

class ImmediateProperties : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("ImmediatePropertyNames", "objectName")
};

namespace ScopedEnumsWithNameClash
{
Q_NAMESPACE

enum class ScopedEnum : int { ScopedVal1, ScopedVal2, ScopedVal3, OtherScopedEnum };
Q_ENUM_NS(ScopedEnum)

enum class OtherScopedEnum : int { ScopedVal1 = 10, ScopedVal2 = 11, ScopedVal3 = 12 };
Q_ENUM_NS(OtherScopedEnum)
};

class ScopedEnumsWithResolvedNameClash
{
    Q_GADGET
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

public:
    enum class ScopedEnum : int { ScopedVal1, ScopedVal2, ScopedVal3, OtherScopedEnum };
    Q_ENUM(ScopedEnum)

    enum class OtherScopedEnum : int { ScopedVal1, ScopedVal2, ScopedVal3 };
    Q_ENUM(OtherScopedEnum)
};

class AttachedType : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(
            QString attachedName READ attachedName WRITE setAttachedName NOTIFY attachedNameChanged)

    QString m_name;

public:
    AttachedType(QObject *parent = nullptr) : QObject(parent) { }

    QString attachedName() const { return m_name; }
    void setAttachedName(const QString &name)
    {
        if (name != m_name) {
            m_name = name;
            Q_EMIT attachedNameChanged();
        }
    }
Q_SIGNALS:
    void attachedNameChanged();
};

class Extension : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int extension READ extension WRITE setExtension NOTIFY extensionChangedWithValue FINAL)

    QML_ATTACHED(AttachedType)
public:
    Extension(QObject *parent = nullptr) : QObject(parent) {}
    int extension() const { return ext; }
    void setExtension(int e) {
        if (e != ext) {
            ext = e;
            emit extensionChanged();
            emit extensionChangedWithValue(e);
        }
    }
    Q_INVOKABLE int invokable() { return 123; }

    static AttachedType *qmlAttachedProperties(QObject *object) { return new AttachedType(object); }

Q_SIGNALS:
    void extensionChanged();
    void extensionChangedWithValue(int value);
public slots:
    int slot() { return 456; }
private:
    int ext = 42;
};

class Extended : public QObject
{
    Q_OBJECT
    QML_EXTENDED(Extension)
    QML_NAMED_ELEMENT(Extended)
    Q_PROPERTY(int base READ base CONSTANT)

public:
    int base() const { return 43; }
};

class Local : public QObject
{
    Q_OBJECT
};

class Foreign
{
    Q_GADGET
    QML_FOREIGN(Local)
    QML_NAMED_ELEMENT(Foreign)
};

class ForeignExtended
{
    Q_GADGET
    QML_FOREIGN(Local)
    QML_NAMED_ELEMENT(ForeignExtended)
    QML_EXTENDED(Extension)
};

class BareSingleton : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT
    QML_ADDED_IN_VERSION(1, 0)

public:
    BareSingleton(QObject *parent = nullptr) : QObject(parent)
    {
        setObjectName("statically registered");
    }
};

class UncreatableSingleton : public QObject
{
    Q_OBJECT
    QML_SINGLETON
    QML_ELEMENT
    QML_ADDED_IN_VERSION(1, 0)

public:
    static UncreatableSingleton *instance();

private:
    UncreatableSingleton() { setObjectName("uncreatable"); }
};

class UncreatableElementNoReason : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")
};

namespace ExtensionNamespace {
Q_NAMESPACE

enum Foo {
    Bar = 9,
    Baz = 12
};
Q_ENUM_NS(Foo)
}

class ExtendedByNamespace : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_EXTENDED_NAMESPACE(ExtensionNamespace)

    Q_PROPERTY(int own READ own CONSTANT)
public:

    enum OwnEnum {
        Moo = 16,
        Maeh = 17
    };
    Q_ENUM(OwnEnum)

    ExtendedByNamespace(QObject *parent = nullptr) : QObject(parent) {}
    int own() const { return 93; }
};

class ExtendedByNamespaceInParent : public ExtendedByNamespace
{
    Q_OBJECT
    QML_ELEMENT
public:
    ExtendedByNamespaceInParent(QObject *parent = nullptr) : ExtendedByNamespace(parent) { }
};

class ExtendedNamespaceByObject : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_EXTENDED_NAMESPACE(Extension)

    Q_PROPERTY(QString dummy READ dummy CONSTANT)
    Q_PROPERTY(int extension READ extension WRITE setExtension NOTIFY extensionChanged)

    int m_ext = 0;

public:
    ExtendedNamespaceByObject(QObject *parent = nullptr) : QObject(parent) {}
    QString dummy() const { return QStringLiteral("dummy"); }
    int extension() const { return m_ext; }
    void setExtension(int e)
    {
        if (e != m_ext) {
            m_ext = e;
            Q_EMIT extensionChanged();
        }
    }

Q_SIGNALS:
    void extensionChanged();
};

class FactorySingleton : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int foo READ foo CONSTANT)
public:

    static FactorySingleton *create(QQmlEngine *, QJSEngine *)
    {
        return new FactorySingleton;
    }

    int foo() const { return 314; }

private:
    FactorySingleton() = default;
};

class ExtendedSingleton : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    QML_EXTENDED(Extension)

    Q_PROPERTY(int foo READ foo CONSTANT)
public:

    int foo() const { return 315; }
};

class NamespaceExtendedSingleton : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    QML_EXTENDED_NAMESPACE(ExtensionNamespace)

    Q_PROPERTY(int foo READ foo CONSTANT)
public:

    int foo() const { return 316; }
};

class ForeignSingleton : public QObject {
    Q_OBJECT
    Q_PROPERTY(int number READ number WRITE setnumber NOTIFY numberchanged)
public:
    ForeignSingleton(QObject *parent = nullptr) : QObject(parent) {};
    int number() { return m_number; }
    void setnumber(int number) { m_number = number; }
    static ForeignSingleton *obtain() { return new ForeignSingleton; }
signals:
    void numberchanged();
private:
    int m_number = 0;
};

class WrapperSingleton : public QObject {
    Q_OBJECT
    QML_NAMED_ELEMENT(ForeignSingleton)
    QML_FOREIGN(ForeignSingleton)
    QML_SINGLETON

public:
    static ForeignSingleton* create(QQmlEngine *, QJSEngine *) {
        ForeignSingleton *singleton = ForeignSingleton::obtain();
        singleton->setnumber(42);
        return singleton;
    }

private:
    WrapperSingleton() = default;
};

class ExtensionA : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int a READ a CONSTANT)
    Q_PROPERTY(int c READ c CONSTANT)
    Q_PROPERTY(int d READ d CONSTANT)
    Q_PROPERTY(int f READ f CONSTANT)
    Q_PROPERTY(int g READ g CONSTANT)
public:
    ExtensionA(QObject *parent = nullptr) : QObject(parent) {}
    int a() const { return 'a'; }
    int c() const { return 11; }
    int d() const { return 21; }
    int f() const { return 31; }
    int g() const { return 41; }
};

class ExtensionB : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY(int b READ b CONSTANT)
    Q_PROPERTY(int c READ c CONSTANT)
    Q_PROPERTY(int d READ d CONSTANT)
public:
    ExtensionB(QObject *parent = nullptr) : QObject(parent) {}
    int b() const { return 'b'; }
    int c() const { return 12; }
    int d() const { return 22; }
};

class IndirectExtensionB : public ExtensionB
{
    Q_OBJECT
    QML_ANONYMOUS
public:
    IndirectExtensionB(QObject *parent = nullptr) : ExtensionB(parent) { }
};

class MultiExtensionParent : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    QML_EXTENDED(ExtensionA)
    Q_PROPERTY(int p READ p CONSTANT)
    Q_PROPERTY(int c READ c CONSTANT)
    Q_PROPERTY(int f READ f CONSTANT)
public:
    MultiExtensionParent(QObject *parent = nullptr) : QObject(parent) {}
    int p() const { return 'p'; }
    int c() const { return 13; }
    int f() const { return 33; }
};

class MultiExtension : public MultiExtensionParent
{
    Q_OBJECT
    QML_ELEMENT
    QML_EXTENDED(ExtensionB)
    Q_PROPERTY(int e READ e CONSTANT)
    Q_PROPERTY(int c READ c CONSTANT)
    Q_PROPERTY(int g READ g CONSTANT)
public:
    MultiExtension(QObject *parent = nullptr) : MultiExtensionParent(parent) {}
    int e() const { return 'e'; }
    int c() const { return 14; }
    int g() const { return 44; }
};

class MultiExtensionIndirect : public MultiExtensionParent
{
    Q_OBJECT
    QML_ELEMENT
    QML_EXTENDED(IndirectExtensionB)

    Q_PROPERTY(int b READ b CONSTANT) // won't be able to use ExtensionB, so provide own property

    Q_PROPERTY(int e READ e CONSTANT)
    Q_PROPERTY(int c READ c CONSTANT)
    Q_PROPERTY(int g READ g CONSTANT)
public:
    MultiExtensionIndirect(QObject *parent = nullptr) : MultiExtensionParent(parent) { }

    int b() const { return 77; }

    int e() const { return 'e'; }
    int c() const { return 'c'; }
    int g() const { return 44; }
};

class ExtendedInParent : public MultiExtensionParent
{
    Q_OBJECT
    QML_ELEMENT
    // properties from base type: p, c, f
    // properties from base type's extension: a, c, d, f, g

    Q_PROPERTY(int c READ c CONSTANT) // be evil: overwrite base type extension's property
public:
    ExtendedInParent(QObject *parent = nullptr) : MultiExtensionParent(parent) { }

    int c()
    {
        Q_UNREACHABLE_RETURN(1111);
    }
};

class ExtendedByIndirect : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_EXTENDED(IndirectExtensionB)
    // properties from extension's base type: b, c, d
public:
    ExtendedByIndirect(QObject *parent = nullptr) : QObject(parent) { }
};

class ExtendedInParentByIndirect : public ExtendedByIndirect
{
    Q_OBJECT
    QML_ELEMENT
    // properties from base type's extension's base type: b, c, d
public:
    ExtendedInParentByIndirect(QObject *parent = nullptr) : ExtendedByIndirect(parent) { }
};

class MultiExtensionThreeExtensions : public MultiExtension
{
    Q_OBJECT
    QML_ELEMENT
    QML_EXTENDED(Extension)
public:
    MultiExtensionThreeExtensions(QObject *parent = nullptr) : MultiExtension(parent) { }
};

class MultiExtensionWithoutExtension : public MultiExtension
{
    Q_OBJECT
    QML_ANONYMOUS
public:
    MultiExtensionWithoutExtension(QObject *parent = nullptr) : MultiExtension(parent) { }
};

class MultiExtensionWithExtensionInBaseBase : public MultiExtensionWithoutExtension
{
    Q_OBJECT
    QML_ELEMENT
    QML_EXTENDED(Extension)
public:
    MultiExtensionWithExtensionInBaseBase(QObject *parent = nullptr)
        : MultiExtensionWithoutExtension(parent)
    {
    }
};

class RevisionedExtension : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int extension READ extension WRITE setExtension REVISION(1, 0))
public:
    RevisionedExtension(QObject *parent = nullptr) : QObject(parent) {}
    int extension() const { return m_ext; }
    void setExtension(int e) { m_ext = e; }
private:
    int m_ext = 42;
};

class ExtendedWithRevisionOld : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_EXTENDED(RevisionedExtension)
    QML_ADDED_IN_VERSION(0, 5)
public:
    ExtendedWithRevisionOld(QObject *parent = nullptr) : QObject(parent) { }
};

class ExtendedWithRevisionNew : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_EXTENDED(RevisionedExtension)
    QML_ADDED_IN_VERSION(1, 0)
public:
    ExtendedWithRevisionNew(QObject *parent = nullptr) : QObject(parent) { }
};

class MyExtendedGroupedObject : public MyGroupedObject
{
    Q_OBJECT
    QML_ANONYMOUS
    QML_EXTENDED(Extension)
    Q_PROPERTY(int value2 READ value2 WRITE setValue2)
    int m_value2 = 0;
public:
    int value2() const { return m_value2; }
    void setValue2(int v) { m_value2 = v; }
};

class ExtendedInGroup : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(MyExtendedGroupedObject *group READ group)

    MyExtendedGroupedObject m_group;
public:
    ExtendedInGroup(QObject *parent = nullptr) : QObject(parent) { }

    MyExtendedGroupedObject *group() { return &m_group; }
};

class StringSignaler : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    StringSignaler(QObject *parent = nullptr) : QObject(parent) {}
    Q_INVOKABLE void call() { emit signal(QJSValue("Hello world!")); }
signals:
    void signal(QJSValue value);
};

class EnumList : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    enum Enum { Alpha, Beta, Gamma };
    Q_ENUM(Enum)

    Q_INVOKABLE QList<EnumList::Enum> list() const { return { Alpha, Beta, Gamma }; }
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

class ObjectTypeHoldingValueType1 : public QObject
{
    Q_OBJECT
    Q_PROPERTY(ValueTypeWithEnum1 vv READ vv WRITE setVv NOTIFY vvChanged)
    Q_PROPERTY(ValueTypeWithEnum1::Quality q READ q CONSTANT)

public:
    ValueTypeWithEnum1 vv() const
    {
        return m_vv;
    }

    void setVv(ValueTypeWithEnum1 vv)
    {
        if (m_vv.quality() == vv.quality())
            return;

        m_vv = vv;
        emit vvChanged(m_vv);
    }

    ValueTypeWithEnum1::Quality q() const { return m_vv.quality(); }

signals:
    void vvChanged(ValueTypeWithEnum1 vv);

private:
    ValueTypeWithEnum1 m_vv;
};

struct ValueTypeWithEnumForeign1
{
    Q_GADGET
    QML_FOREIGN(ValueTypeWithEnum1)
    QML_NAMED_ELEMENT(valueTypeWithEnum1)
};

namespace ValueTypeWithEnumForeignNamespace1
{
    Q_NAMESPACE
    QML_FOREIGN_NAMESPACE(ValueTypeWithEnum1)
    QML_NAMED_ELEMENT(ValueTypeWithEnum1)
};

struct ObjectTypeHoldingValueTypeForeign1
{
    Q_GADGET
    QML_FOREIGN(ObjectTypeHoldingValueType1)
    QML_NAMED_ELEMENT(ObjectTypeHoldingValueType1)
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

class ObjectTypeHoldingValueType2 : public QObject
{
    Q_OBJECT
    Q_PROPERTY(ValueTypeWithEnum2 vv READ vv WRITE setVv NOTIFY vvChanged)
    Q_PROPERTY(ValueTypeWithEnum2::Quality q READ q CONSTANT)
    Q_CLASSINFO("RegisterEnumsFromRelatedTypes", "false")

public:
    ValueTypeWithEnum2 vv() const
    {
        return m_vv;
    }

    void setVv(ValueTypeWithEnum2 vv)
    {
        if (m_vv.quality() == vv.quality())
            return;

        m_vv = vv;
        emit vvChanged(m_vv);
    }

    ValueTypeWithEnum2::Quality q() const { return m_vv.quality(); }

signals:
    void vvChanged(ValueTypeWithEnum2 vv);

private:
    ValueTypeWithEnum2 m_vv;
};

struct ValueTypeWithEnumForeign2
{
    Q_GADGET
    QML_FOREIGN(ValueTypeWithEnum2)
    QML_NAMED_ELEMENT(valueTypeWithEnum2)
};

namespace ValueTypeWithEnumForeignNamespace2
{
    Q_NAMESPACE
    QML_FOREIGN_NAMESPACE(ValueTypeWithEnum2)
    QML_NAMED_ELEMENT(ValueTypeWithEnum2)
};

struct ObjectTypeHoldingValueTypeForeign2
{
    Q_GADGET
    QML_FOREIGN(ObjectTypeHoldingValueType2)
    QML_NAMED_ELEMENT(ObjectTypeHoldingValueType2)
};

struct Large {
    Q_GADGET
    QML_VALUE_TYPE(large)

    Q_PROPERTY(uint a MEMBER a)
    Q_PROPERTY(uint b MEMBER b)
    Q_PROPERTY(uint c MEMBER c)
    Q_PROPERTY(uint d MEMBER d)
    Q_PROPERTY(uint e MEMBER e)
    Q_PROPERTY(uint f MEMBER f)

public:
    quint64 a;
    quint64 b;
    quint64 c;
    quint64 d;
    quint64 e;
    quint64 f;
};

inline bool operator==(const Large &a, const Large &b)
{
    return a.a == b.a && a.b == b.b && a.c == b.c && a.d == b.d && a.e == b.e && a.f == b.f;
}

inline bool operator!=(const Large &a, const Large &b) { return !(a == b); }

class Foo: public QObject {

    Q_OBJECT
    Q_PROPERTY(QVariantList fooProperty READ getList WRITE setList)
    Q_PROPERTY(Large a MEMBER a BINDABLE aBindable)
    Q_PROPERTY(Large b MEMBER b BINDABLE bBindable)
    QML_ELEMENT

public:
    QVariantList getList() const { return mFooProperty;}
    void setList(QVariantList list) { mFooProperty = list;}

    QBindable<Large> aBindable() { return QBindable<Large>(&a); }
    QBindable<Large> bBindable() { return QBindable<Large>(&b); }

private:
    QProperty<Large> a;
    QProperty<Large> b;
    QVariantList mFooProperty;
};

struct BaseValueType
{
    Q_GADGET
    Q_PROPERTY(int content READ content WRITE setContent)
    QML_VALUE_TYPE(base)

public:
    Q_INVOKABLE void increment() { ++m_content; }
    Q_INVOKABLE QString report() const { return QString::number(m_content); }

    int content() const { return m_content; }
    void setContent(int content) { m_content = content; }

private:
    int m_content = 27;
};

struct DerivedValueType : public BaseValueType
{
    Q_GADGET
    QML_VALUE_TYPE(derived)
public:
    DerivedValueType() { increment(); }
    Q_INVOKABLE int nothing() const { return m_nothing; }

private:
    int m_nothing = 12;
};

class ItemAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString attachedName READ attachedName WRITE setAttachedName NOTIFY attachedNameChanged)
    QML_ELEMENT
    QML_ATTACHED(ItemAttached)
public:
    ItemAttached(QObject *parent = nullptr) : QObject(parent) {}

    QString attachedName() const { return m_name; }
    void setAttachedName(const QString &name)
    {
        if (name != m_name) {
            m_name = name;
            emit attachedNameChanged();
        }
    }

    static ItemAttached *qmlAttachedProperties(QObject *object)
    {
        if (object->objectName() != QLatin1String("foo")) {
            qWarning("Only foo can have ItemAttached!");
            return nullptr;
        }

        return new ItemAttached(object);
    }

signals:
    void attachedNameChanged();

private:
    QString m_name;
};

class BindableOnly : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int score BINDABLE scoreBindable READ default WRITE default FINAL)
    Q_PROPERTY(QByteArray data READ default WRITE default BINDABLE dataBindable FINAL)
    QML_ELEMENT
public:
    BindableOnly(QObject *parent = nullptr)
        : QObject(parent)
        , m_score(4)
    {}
    QBindable<int> scoreBindable() { return QBindable<int>(&m_score); }
    QBindable<QByteArray> dataBindable() { return QBindable<QByteArray>(&m_data); }

private:
    QProperty<int> m_score;
    QProperty<QByteArray> m_data;
};

void registerTypes();

class AttachMe : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool abc READ abc WRITE setAbc NOTIFY abcChanged)
    QML_ANONYMOUS

    bool m_abc;
signals:
    void abcChanged();

public:
    AttachMe(QObject *parent) : QObject(parent) { }
    bool abc() const { return m_abc; }
    void setAbc(bool abc) { m_abc = abc; }
};

class AnotherAttachMe : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString anotherAbc READ anotherAbc WRITE setAnotherAbc NOTIFY anotherAbcChanged)
    QML_ANONYMOUS

    QString m_anotherAbc;
signals:
    void anotherAbcChanged();

public:
    AnotherAttachMe(QObject *parent) : QObject(parent) { }
    QString anotherAbc() const { return m_anotherAbc; }
    void setAnotherAbc(const QString &abc) { m_anotherAbc = abc; }
};

class OriginalQmlAttached : public QObject
{
    Q_OBJECT
    QML_ATTACHED(AttachMe)
    QML_ELEMENT

public:
    static AttachMe *qmlAttachedProperties(QObject *object) { return new AttachMe(object); }
};

class LeakingQmlAttached : public OriginalQmlAttached
{
    Q_OBJECT
    QML_ELEMENT
};

class DerivedQmlAttached : public OriginalQmlAttached
{
    Q_OBJECT
    QML_ELEMENT
    QML_ATTACHED(AnotherAttachMe)

public:
    static AnotherAttachMe *qmlAttachedProperties(QObject *object)
    {
        return new AnotherAttachMe(object);
    }
};

class OriginalSingleton : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

private:
    Q_PROPERTY(QString abc READ abc WRITE setAbc NOTIFY abcChanged)

    QString m_abc;
signals:
    void abcChanged(const QString &);

public:
    Q_INVOKABLE int mm() { return 5; }
    QString abc() const { return m_abc; }
    void setAbc(const QString &abc)
    {
        m_abc = abc;
        emit abcChanged(abc);
    }
};

class LeakingSingleton : public OriginalSingleton
{
    Q_OBJECT
    QML_ELEMENT
};

class DerivedSingleton : public OriginalSingleton
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QString anotherAbc READ anotherAbc WRITE setAnotherAbc NOTIFY anotherAbcChanged)

    QString m_anotherAbc;
signals:
    void anotherAbcChanged(const QString &);

public:
    QString anotherAbc() const { return m_anotherAbc; }
    void setAnotherAbc(const QString &abc)
    {
        m_anotherAbc = abc;
        emit anotherAbcChanged(abc);
    }
};

class Foreigner : public QObject
{
    Q_OBJECT

private:
    Q_PROPERTY(QString abc READ abc WRITE setAbc NOTIFY abcChanged)

    QString m_abc;
signals:
    void abcChanged(const QString &);

public:
    QString abc() const { return m_abc; }
    void setAbc(const QString &abc)
    {
        m_abc = abc;
        emit abcChanged(abc);
    }
};

class ForeignerForeign
{
    Q_GADGET
    QML_ELEMENT
    QML_FOREIGN(Foreigner)
};

class LeakingForeignerForeign : public QObject, public ForeignerForeign
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString anotherAbc READ anotherAbc WRITE setAnotherAbc NOTIFY anotherAbcChanged)

    QString m_anotherAbc;
signals:
    void anotherAbcChanged(const QString &);

public:
    QString anotherAbc() const { return m_anotherAbc; }
    void setAnotherAbc(const QString &abc)
    {
        m_anotherAbc = abc;
        emit anotherAbcChanged(abc);
    }
};


struct ForeignNamespace
{
    Q_GADGET
public:
    enum Abc { A, B, C, D };
    Q_ENUM(Abc)
};

class ForeignNamespaceForeign
{
    Q_GADGET
    QML_ELEMENT
    QML_FOREIGN_NAMESPACE(ForeignNamespace)
};

class LeakingForeignNamespaceForeign : public QObject, public ForeignNamespaceForeign
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum AnotherAbc { D, C, B, A };
    Q_ENUM(AnotherAbc)
};

struct ValueTypeWithLength
{
    Q_GADGET
    QML_VALUE_TYPE(withLength)
    QML_CONSTRUCTIBLE_VALUE

    Q_PROPERTY(int length READ length CONSTANT)

public:
    ValueTypeWithLength() = default;
    Q_INVOKABLE ValueTypeWithLength(int length) : m_length(length) {}
    Q_INVOKABLE QString toString() const { return QStringLiteral("no"); }

    int length() const { return m_length; }

private:
    int m_length = 19;
};

struct ValueTypeWithString
{
    Q_GADGET
    QML_VALUE_TYPE(withString)
    QML_CONSTRUCTIBLE_VALUE

public:
    Q_INVOKABLE ValueTypeWithString(const QString &v = QString()) : m_string(v) {}
    QString toString() const { return m_string; }

private:
    QString m_string;
};

class GetterObject : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    explicit GetterObject(QObject *parent = nullptr) : QObject{parent} {}

    // always returns a 0 as uint64_t
    Q_INVOKABLE uint64_t getFalse() const { return 0; }
    Q_INVOKABLE uint64_t getTrue() const { return 1; }

    Q_INVOKABLE quint64 getQFalse() const { return 0; }
    Q_INVOKABLE quint64 getQTrue() const { return 1; }
};

class EnumProviderSingleton : public QObject {
    Q_OBJECT

public:
    enum class Expected {
        Value = 42
    };
    Q_ENUM(Expected)

    EnumProviderSingleton(QObject* parent = nullptr) : QObject(parent) {}
};

class EnumProviderSingletonQml {
    Q_GADGET
    QML_FOREIGN(EnumProviderSingleton)
    QML_NAMED_ELEMENT(EnumProviderSingleton)
    QML_SINGLETON

public:
    static EnumProviderSingleton* create(QQmlEngine*, QJSEngine*) {
        return new EnumProviderSingleton();
    }

private:
    EnumProviderSingletonQml() = default;
};



namespace TypedEnums {
Q_NAMESPACE
QML_ELEMENT

enum E8S : qint8 {
    E8SA = std::numeric_limits<qint8>::min(),
    E8SB = -5,
    E8SC = -1,
    E8SD = 0,
    E8SE = 1,
    E8SF = 5,
    E8SG = std::numeric_limits<qint8>::max(),
};
Q_ENUM_NS(E8S);

enum E8U : quint8 {
    E8UA = 0,
    E8UB = 1,
    E8UC = 5,
    E8UD = 1 << 7,
    E8UE = std::numeric_limits<quint8>::max(),
};
Q_ENUM_NS(E8U);

enum E16S : qint16 {
    E16SA = std::numeric_limits<qint16>::min(),
    E16SB = -5,
    E16SC = -1,
    E16SD = 0,
    E16SE = 1,
    E16SF = 5,
    E16SG = std::numeric_limits<qint16>::max(),
};
Q_ENUM_NS(E16S);

enum E16U : quint16 {
    E16UA = 0,
    E16UB = 1,
    E16UC = 5,
    E16UD = 1 << 15,
    E16UE = std::numeric_limits<quint16>::max(),
};
Q_ENUM_NS(E16U);

enum E32S : qint32 {
    E32SA = std::numeric_limits<qint32>::min(),
    E32SB = -5,
    E32SC = -1,
    E32SD = 0,
    E32SE = 1,
    E32SF = 5,
    E32SG = std::numeric_limits<qint32>::max(),
};
Q_ENUM_NS(E32S);

enum E32U : quint32 {
    E32UA = 0,
    E32UB = 1,
    E32UC = 5,
    E32UD = 1u << 31,
    E32UE = std::numeric_limits<quint32>::max(),
};
Q_ENUM_NS(E32U);

enum E64S : qint64 {
    E64SA = std::numeric_limits<qint64>::min(),
    E64SB = -5,
    E64SC = -1,
    E64SD = 0,
    E64SE = 1,
    E64SF = 5,
    E64SG = std::numeric_limits<qint64>::max(),
};
Q_ENUM_NS(E64S);

enum E64U : quint64 {
    E64UA = 0,
    E64UB = 1,
    E64UC = 5,
    E64UD = 1ull << 63,
    E64UE = std::numeric_limits<quint64>::max(),
};
Q_ENUM_NS(E64U);
}

class GadgetWithEnums
{
    Q_GADGET
    QML_VALUE_TYPE(gadgetWithEnums)
    Q_PROPERTY(TypedEnums::E8S  e8s  MEMBER m_e8s);
    Q_PROPERTY(TypedEnums::E8U  e8u  MEMBER m_e8u);
    Q_PROPERTY(TypedEnums::E16S e16s MEMBER m_e16s);
    Q_PROPERTY(TypedEnums::E16U e16u MEMBER m_e16u);
    Q_PROPERTY(TypedEnums::E32S e32s MEMBER m_e32s);
    Q_PROPERTY(TypedEnums::E32U e32u MEMBER m_e32u);
    Q_PROPERTY(TypedEnums::E64S e64s MEMBER m_e64s);
    Q_PROPERTY(TypedEnums::E64U e64u MEMBER m_e64u);
public:
    TypedEnums::E8S  m_e8s  = {};
    TypedEnums::E8U  m_e8u  = {};
    TypedEnums::E16S m_e16s = {};
    TypedEnums::E16U m_e16u = {};
    TypedEnums::E32S m_e32s = {};
    TypedEnums::E32U m_e32u = {};
    TypedEnums::E64S m_e64s = {};
    TypedEnums::E64U m_e64u = {};
private:
    friend bool operator==(const GadgetWithEnums &a, const GadgetWithEnums &b)
    {
        return a.m_e8s == b.m_e8s && a.m_e8u == b.m_e8u && a.m_e16s == b.m_e16s
                && a.m_e16u == b.m_e16u && a.m_e32s == b.m_e32s && a.m_e32u == b.m_e32u
                && a.m_e64s == b.m_e64s && a.m_e64u == b.m_e64u;
    }
    friend bool operator!=(const GadgetWithEnums &a, const GadgetWithEnums &b)
    {
        return !(a == b);
    }
};

class ObjectWithEnums : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(TypedEnums::E8S  e8s  MEMBER m_e8s  NOTIFY changed);
    Q_PROPERTY(TypedEnums::E8U  e8u  MEMBER m_e8u  NOTIFY changed);
    Q_PROPERTY(TypedEnums::E16S e16s MEMBER m_e16s NOTIFY changed);
    Q_PROPERTY(TypedEnums::E16U e16u MEMBER m_e16u NOTIFY changed);
    Q_PROPERTY(TypedEnums::E32S e32s MEMBER m_e32s NOTIFY changed);
    Q_PROPERTY(TypedEnums::E32U e32u MEMBER m_e32u NOTIFY changed);
    Q_PROPERTY(TypedEnums::E64S e64s MEMBER m_e64s NOTIFY changed);
    Q_PROPERTY(TypedEnums::E64U e64u MEMBER m_e64u NOTIFY changed);
    Q_PROPERTY(GadgetWithEnums g MEMBER m_g NOTIFY changed);
public:
    ObjectWithEnums(QObject *parent = nullptr) : QObject(parent) {}
    TypedEnums::E8S  m_e8s  = {};
    TypedEnums::E8U  m_e8u  = {};
    TypedEnums::E16S m_e16s = {};
    TypedEnums::E16U m_e16u = {};
    TypedEnums::E32S m_e32s = {};
    TypedEnums::E32U m_e32u = {};
    TypedEnums::E64S m_e64s = {};
    TypedEnums::E64U m_e64u = {};
    GadgetWithEnums m_g;
Q_SIGNALS:
    void changed();
};

struct UnregisteredValueBaseType
{
    int foo = 12;
};

struct UnregisteredValueDerivedType: public UnregisteredValueBaseType
{
    int bar = 13;
};

struct GadgetedValueBaseType
{
    Q_GADGET
    int foo = 12;
};

struct GadgetedValueDerivedType: public GadgetedValueBaseType
{
    Q_GADGET
    int bar = 13;
};

class UnregisteredValueTypeHandler: public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    int consumed = 0;
    int gadgeted = 0;

public slots:
    UnregisteredValueBaseType produce() { return UnregisteredValueBaseType(); }
    UnregisteredValueDerivedType produceDerived() { return UnregisteredValueDerivedType(); }
    void consume(UnregisteredValueBaseType) { ++consumed; }

    GadgetedValueDerivedType produceGadgeted() { return GadgetedValueDerivedType(); }
    void consume(GadgetedValueBaseType) { ++gadgeted; }
};

class Greeter : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    Greeter(QObject *parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE void greet()
    {
        qDebug().noquote() << objectName() << "says hello";
    }

    Q_INVOKABLE void sum(int a, int b)
    {
        qDebug().noquote() << objectName() << QString("says %1 + %2 = %3").arg(a).arg(b).arg(a + b);
    }
};

class Attachment : public QObject {
    Q_OBJECT
public:
    Attachment(QObject *parent = nullptr) : QObject(parent) {}
};

class AttachedInCtor : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_ATTACHED(Attachment)

public:
    AttachedInCtor(QObject *parent = nullptr)
        : QObject(parent)
    {
        attached = qmlAttachedPropertiesObject<AttachedInCtor>(this, true);
    }

    static Attachment *qmlAttachedProperties(QObject *object) {
        return new Attachment(object);
    }

    QObject *attached = nullptr;
};

class BirthdayParty : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<QObject> guests READ guests)
    Q_CLASSINFO("DefaultProperty", "guests")
    QML_ELEMENT

public:
    using QObject::QObject;
    QQmlListProperty<QObject> guests() { return {this, &m_guests}; }
    qsizetype guestCount() const { return m_guests.count(); }
    QObject *guest(qsizetype i) const { return m_guests.at(i); }

private:
    QList<QObject *> m_guests;
};

class ByteArrayReceiver : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    QList<QByteArray> byteArrays;

    Q_INVOKABLE void byteArrayTest(const QByteArray &ba)
    {
        byteArrays.push_back(ba);
    }
};

class CounterAttachedBaseType: public QObject
{
    Q_OBJECT
    QML_ANONYMOUS
    Q_PROPERTY (int value READ value NOTIFY valueChanged)

public:
    CounterAttachedBaseType(QObject *parent = nullptr) : QObject(parent) {}

    int value() { return m_value; }
    Q_SIGNAL void valueChanged();

protected:
    int m_value = 98;
};


class CounterAttachedType: public CounterAttachedBaseType
{
    Q_OBJECT
    QML_ANONYMOUS

public:
    CounterAttachedType(QObject *parent = nullptr) : CounterAttachedBaseType(parent) {}

    Q_INVOKABLE void increase() {
        ++m_value;
        Q_EMIT valueChanged();
    }
};

class Counter : public QObject
{
    Q_OBJECT
    QML_ATTACHED(CounterAttachedBaseType)
    QML_ELEMENT

public:
    static CounterAttachedBaseType *qmlAttachedProperties(QObject *o)
    {
        return new CounterAttachedType(o);
    }
};


class Singleton: public QObject
{
    Q_OBJECT
    Q_PROPERTY(EnumType enumProperty READ enumProperty CONSTANT)
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")
    QML_ELEMENT
    QML_SINGLETON
public:
    explicit Singleton(QObject* parent = nullptr) : QObject(parent) {}
    enum class EnumType {
        EnumValue1,
        EnumValue2
    };
    Q_ENUM(EnumType);
    EnumType enumProperty() const {
        return EnumType::EnumValue2;
    }
};

class NonSingleton: public QObject
{
    Q_OBJECT
    Q_PROPERTY(EnumType enumProperty READ enumProperty CONSTANT)
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")
    QML_ELEMENT
public:
    explicit NonSingleton(QObject* parent = nullptr) : QObject(parent) {}
    enum class EnumType {
        EnumValue1,
        EnumValue2
    };
    Q_ENUM(EnumType);
    EnumType enumProperty() const {
        return EnumType::EnumValue2;
    }
};

class TypeWithQJsonArrayProperty : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QJsonArray jsonArray READ jsonArray WRITE setJsonArray NOTIFY jsonArrayChanged)

public:
    TypeWithQJsonArrayProperty(QObject *parent = nullptr) : QObject(parent) {}

    const QJsonArray& jsonArray() { return m_jsonArray; }
    void setJsonArray(const QJsonArray& a) { m_jsonArray = a; }

signals:
    void jsonArrayChanged();

private:
    QJsonArray m_jsonArray;
};

class InvokableSingleton : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    InvokableSingleton() = default;
    Q_INVOKABLE InvokableSingleton(int a, QObject *parent) : QObject(parent), m_a(a) {}

    int m_a = 0;
};

class InvokableExtension : public QObject
{
    Q_OBJECT
public:
    Q_INVOKABLE InvokableExtension(QObject *parent = nullptr) : QObject(parent) {}
};

class InvokableExtended : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_EXTENDED(InvokableExtension)

public:
    Q_INVOKABLE InvokableExtended() = default;
};

class InvokableUncreatable : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("no")

public:
    Q_INVOKABLE InvokableUncreatable() = default;
};

class InvokableValueType
{
    Q_GADGET
    QML_VALUE_TYPE(vv)
public:
    Q_INVOKABLE InvokableValueType() = default;
    Q_INVOKABLE InvokableValueType(const QString &s) : m_s(s) {}
    QString m_s;
};

class NestedVectors : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    NestedVectors(QObject *parent = nullptr) : QObject(parent)
    {
        std::vector<int> data;
        data.push_back(1);
        data.push_back(2);
        data.push_back(3);
        m_list.push_back(data);
        data.clear();
        data.push_back(4);
        data.push_back(5);
        m_list.push_back(data);
    }

    Q_INVOKABLE std::vector<std::vector<int>> getList()
    {
        return m_list;
    }

    Q_INVOKABLE void setList(std::vector<std::vector<int>> list)
    {
        m_list = list;
    }

private:
    std::vector<std::vector<int>> m_list;
};

class VariantAssociationProvider : public QObject {

    Q_OBJECT
    Q_PROPERTY(QVariantMap variantMap READ getMap WRITE setMap)
    Q_PROPERTY(QVariantHash variantHash READ getHash WRITE setHash)
    Q_PROPERTY(QVariantList variantList READ getList WRITE setList)

    QML_ELEMENT

    QVariantMap m_variantMap;
    QVariantHash m_variantHash;
    QVariantList m_variantList;

public:
    VariantAssociationProvider(QObject* parent = nullptr) : QObject(parent)
    {}

    QVariantMap getMap() { return m_variantMap; }
    void setMap(const QVariantMap& map) { m_variantMap = map; }

    QVariantHash getHash() { return m_variantHash; }
    void setHash(const QVariantHash& hash) { m_variantHash = hash; }

    QVariantList getList() { return m_variantList; }
    void setList(const QVariantList &list) { m_variantList = list; }

    Q_INVOKABLE QVariantList getListOfMap() const {
        return QVariantList{
            QVariantMap{},
            QVariantMap{ { "email", "Alice Smith"} }
        };
    }

    Q_INVOKABLE QVariantList getListOfHash() const {
        return QVariantList{
            QVariantHash{},
            QVariantHash{ { "email", "Alice Smith"} }
        };
    }
};

namespace YepNamespaceA {
class YepAttached : public QObject
{
    Q_OBJECT
    QML_ANONYMOUS

public:
    YepAttached(QObject *parent) : QObject(parent) { }
    Q_INVOKABLE QString s() const { return QStringLiteral("StaticTest Attached Type"); }
};
class Yep : public QObject
{
    Q_OBJECT
    QML_ATTACHED(YepAttached)
    QML_ELEMENT

public:
    static YepAttached *qmlAttachedProperties(QObject *object) { return new YepAttached(object); }
};

class YepSingleton : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    Q_INVOKABLE QString s() const { return QStringLiteral("StaticTest Singleton"); }
};

class MyObject : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    Q_INVOKABLE QString s() const { return QStringLiteral("StaticTest"); }
};
} // namespace YepNamespaceA

class ReadCounterInner {
    Q_GADGET
    QML_ELEMENT

    Q_PROPERTY(QDateTime firstDate READ getFirstDate WRITE setFirstDate)
    Q_PROPERTY(QDateTime secondDate READ getSecondDate WRITE setSecondDate)

public:
    ReadCounterInner() {}

    inline static std::size_t timesRead = 0;

    QDateTime getFirstDate() {
        ++timesRead;
        return m_firstDate;
    }
    void setFirstDate(const QDateTime& l) { m_firstDate = l; }

    QDateTime getSecondDate() {
        ++timesRead;
        return m_secondDate;
    }
    void setSecondDate(const QDateTime& l) { m_secondDate = l; }


    QDateTime m_firstDate{};
    QDateTime m_secondDate{};
};

class ReadCounter : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QStringList stringList READ getStringList WRITE setStringList NOTIFY stringListChanged)
    Q_PROPERTY(QDateTime dateTime READ getDateTime WRITE setDateTime NOTIFY dateTimeChanged)
    Q_PROPERTY(ValueTypeWithEnum1 valueType READ getValueType WRITE setValueType NOTIFY valueTypeChanged)
    Q_PROPERTY(QStringList bindable READ getBindable WRITE default BINDABLE bindableProperty FINAL)
    Q_PROPERTY(ReadCounterInner inner READ getInner WRITE setInner NOTIFY innerChanged)
    Q_PROPERTY(QStringList notifyBindable READ default WRITE default BINDABLE notifyBindableProperty NOTIFY notifyBindableChanged)

public:
    ReadCounter(QObject* parent = nullptr)
        : QObject(parent)
    {}

    std::size_t timesRead = 0;

    QStringList getStringList() {
        ++timesRead;
        return m_stringList;
    }
    void setStringList(const QStringList& l) { m_stringList = l; }

    QDateTime getDateTime() {
        ++timesRead;
        return m_dateTime;
    }
    void setDateTime(const QDateTime& d) { m_dateTime = d; }

    ValueTypeWithEnum1 getValueType() {
        ++timesRead;
        return m_valueType;
    }
    void setValueType(const ValueTypeWithEnum1& v) { m_valueType = v; emit valueTypeChanged(); }

    QBindable<QStringList> bindableProperty() { return QBindable<QStringList>(&m_bindable); }
    QStringList getBindable() {
        ++timesRead;
        return m_bindable;
    }

    ReadCounterInner getInner() {
        ++timesRead;
        return m_inner;
    }
    void setInner(const ReadCounterInner& l) { m_inner = l; emit innerChanged(); }

    QBindable<QStringList> notifyBindableProperty() { return QBindable<QStringList>(&m_notifyBindable); }

    std::size_t destroyedConnections = 0;
    std::size_t notifyBindableSignalConnections = 0;

    void connectNotify(const QMetaMethod& signal) override {
        if (signal == QMetaMethod::fromSignal(&ReadCounter::destroyed))
            ++destroyedConnections;
        if (signal == QMetaMethod::fromSignal(&ReadCounter::notifyBindableChanged))
            ++notifyBindableSignalConnections;
    }

    void disconnectNotify(const QMetaMethod& signal) override {
        if (signal == QMetaMethod::fromSignal(&ReadCounter::destroyed))
            --destroyedConnections;
        if (signal == QMetaMethod::fromSignal(&ReadCounter::notifyBindableChanged))
            --notifyBindableSignalConnections;
    }

signals:
    void stringListChanged();
    void dateTimeChanged();
    void valueTypeChanged();
    void innerChanged();
    void notifyBindableChanged();

private:
    QStringList m_stringList;
    QDateTime m_dateTime;
    ValueTypeWithEnum1 m_valueType;
    QProperty<QStringList> m_bindable;
    ReadCounterInner m_inner{};
    QProperty<QStringList> m_notifyBindable;
};

class BindablePoint : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QPointF point BINDABLE bindablePoint READ default WRITE default)
public:
    BindablePoint(QObject *parent = nullptr) : QObject(parent), m_point(QPointF(101, 102)) {}

    QBindable<QPointF> bindablePoint() { return QBindable<QPointF>(&m_point); }

private:
    QProperty<QPointF> m_point;
};

class LargeRevisionBase : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int c MEMBER m_c CONSTANT)
    Q_PROPERTY(int d MEMBER m_d CONSTANT)
public:
    int m_c = 11;
    int m_d = 12;
};

class LargeRevision : public LargeRevisionBase
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int a MEMBER m_a CONSTANT REVISION(1, 12))
    Q_PROPERTY(int b MEMBER m_b CONSTANT REVISION(12, 12))
    Q_PROPERTY(int c MEMBER m_c CONSTANT REVISION(1, 12))
    Q_PROPERTY(int d MEMBER m_d CONSTANT REVISION(12, 12))
public:
    int m_a = 13;
    int m_b = 14;
    int m_c = 15;
    int m_d = 16;
};

class CppEnum : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum class Scoped { S1, S2, S3 = 5, S4 = 5 };
    Q_ENUM(Scoped)
    enum Unscoped { U1, U2, U3 = 5, U4 = 5 };
    Q_ENUM(Unscoped)
};

namespace EnumNamespace {
Q_NAMESPACE
QML_ELEMENT

enum Unscoped { U1, U2, U3 = 5, U4 = 5 };
Q_ENUM_NS(Unscoped)
enum class Scoped { S1, S2, S3 = 5, S4 = 5 };
Q_ENUM_NS(Scoped)
} // namespace EnumNamespace

class ConflictingEnums : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum class E1 { A = 1 };
    Q_ENUM(E1);
    enum E2 { A = 2 };
    Q_ENUM(E2);
};

template <class T>
class IdType
{
public:
    IdType(T id) : value(id) {}
    T get() const { return value; }

private:
    T value;
};

using CustomId = IdType<int32_t>;

class CustomIdentifier
{
    Q_GADGET
    QML_VALUE_TYPE(customIdentifier)
    QML_STRUCTURED_VALUE

    Q_PROPERTY(int customId READ getCustomId CONSTANT FINAL)
public:
    CustomIdentifier() = default;
    CustomIdentifier(CustomId custom_id) : id(std::move(custom_id)) {}

    int getCustomId() const { return id.get(); }
    Q_INVOKABLE int toVisualId() const { return id.get() + 1; }

    friend bool operator==(const CustomIdentifier &a, const CustomIdentifier &b)
    {
        return a.id.get() == b.id.get();
    }

    CustomId id {-1};
};

class IdProvider : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    IdProvider() = default;

    static CustomId id1() { return CustomId(20); }
    static CustomId id2() { return CustomId(21); }

    Q_INVOKABLE CustomIdentifier getId1() const { return CustomIdentifier(id1()); }
    Q_INVOKABLE CustomIdentifier getId2() const { return CustomIdentifier(id2()); }
};

#endif // TESTTYPES_H
