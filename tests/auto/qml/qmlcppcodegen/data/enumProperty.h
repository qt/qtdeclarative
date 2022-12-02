#ifndef ENUMPROPERTY_H
#define ENUMPROPERTY_H

#include <QObject>
#include <QtQml>

class MyEnumType
{
    Q_GADGET
    QML_ANONYMOUS
public:
    enum MyEnum {
        Sin = 0x01,
        Saw = 0x02,
        Tri = 0x04,
    };
    Q_ENUM(MyEnum)
    Q_PROPERTY(MyEnum type READ type)
    MyEnum type() const { return MyEnum::Tri; }
};

class MyType : public QObject
{
    Q_OBJECT
    Q_PROPERTY(MyEnumType myEnumType READ myEnumType)
    QML_ELEMENT
public:
    MyEnumType myEnumType() const { return m_type; }

private:
    MyEnumType m_type;
};

#endif // ENUMPROPERTY_H
