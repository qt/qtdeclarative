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
    Q_PROPERTY(MyEnum type READ type CONSTANT)
    MyEnum type() const { return MyEnum::Tri; }
};

class MyType : public QObject
{
    Q_OBJECT
    Q_PROPERTY(MyEnumType myEnumType READ myEnumType CONSTANT)
    Q_PROPERTY(A a READ a WRITE setA NOTIFY aChanged FINAL)
    QML_ELEMENT
public:
    enum A { B, C, D };
    Q_ENUM(A)

    MyEnumType myEnumType() const { return m_type; }

    A a() const { return m_a; }
    void setA(A newA)
    {
        if (m_a == newA)
            return;
        m_a = newA;
        emit aChanged();
    }

    Q_INVOKABLE int method(quint16, const QString &) { return 24; }
    Q_INVOKABLE int method(quint16, MyType::A a) { return int(a); }

Q_SIGNALS:
    void aChanged();

private:
    MyEnumType m_type;
    A m_a = B;
};

#endif // ENUMPROPERTY_H
