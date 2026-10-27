#ifndef ENUMSHADOWING_H
#define ENUMSHADOWING_H

#include <QObject>
#include <QtQml>

class EnumShadowingMixedBase : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum BaseEnum { K = 1 };
    Q_ENUM(BaseEnum)
};

class EnumShadowingCppBase : public QObject
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum BaseEnum { K = 1 };
    Q_ENUM(BaseEnum)
};

class EnumShadowingCppDerived : public EnumShadowingCppBase
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum DerivedEnum { K = 2 };
    Q_ENUM(DerivedEnum)
};

#endif // ENUMSHADOWING_H
