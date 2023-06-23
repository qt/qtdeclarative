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

class CommunicationPermission
{
    Q_GADGET
public:
    enum CommunicationMode : quint8 {
        Access = 0x01,
        Advertise = 0x02,
        Default = Access | Advertise,
    };
    Q_DECLARE_FLAGS(CommunicationModes, CommunicationMode)
    Q_FLAG(CommunicationModes)

    void setCommunicationModes(CommunicationModes modes) { m_modes = modes; }
    CommunicationModes communicationModes() const { return m_modes; }

private:
    CommunicationModes m_modes;
};

struct QQmlCommunicationPermission : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(CommunicationPermission)
    QML_EXTENDED_NAMESPACE(CommunicationPermission)
    Q_PROPERTY(CommunicationPermission::CommunicationModes communicationModes READ communicationModes WRITE setCommunicationmodes NOTIFY communicationModesChanged)

public:
    CommunicationPermission::CommunicationModes communicationModes() const
    {
        return m_permission.communicationModes();
    }

    void setCommunicationmodes(const CommunicationPermission::CommunicationModes &newCommunicationModes)
    {
        if (communicationModes() == newCommunicationModes)
            return;
        m_permission.setCommunicationModes(newCommunicationModes);
        emit communicationModesChanged();
    }

signals:
    void communicationModesChanged();

private:
    CommunicationPermission m_permission;
};

#endif // ENUMPROPERTY_H
