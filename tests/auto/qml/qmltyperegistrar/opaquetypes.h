// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef OPAQUETYPES_H
#define OPAQUETYPES_H

#include <QtCore/qobject.h>
#include <QtQml/qqmlregistration.h>

// Not registered: only reachable as the base of an opaque type. Since the
// derived type is opaque, this one should be opaque as well.
class OpaqueGrandBase : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int g MEMBER m_g CONSTANT)
public:
    OpaqueGrandBase(QObject *parent = nullptr) : QObject(parent) {}
private:
    int m_g = 1;
};

// Not registered: only used as a property type, so it becomes opaque.
class OpaqueWithOpaqueBase : public OpaqueGrandBase
{
    Q_OBJECT
    Q_PROPERTY(int o MEMBER m_o CONSTANT)
public:
    OpaqueWithOpaqueBase(QObject *parent = nullptr) : OpaqueGrandBase(parent) {}
private:
    int m_o = 2;
};

// Registered as a proper QML type. Even though it is the base of an opaque
// type, it must show up as a normal, registered type and not as opaque.
class RegisteredBase : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int r MEMBER m_r CONSTANT)
public:
    RegisteredBase(QObject *parent = nullptr) : QObject(parent) {}
private:
    int m_r = 3;
};

// Not registered: only used as a property type, so it becomes opaque. Its base
// is a registered type though, so the base must not be turned opaque.
class OpaqueWithRegisteredBase : public RegisteredBase
{
    Q_OBJECT
    Q_PROPERTY(int w MEMBER m_w CONSTANT)
public:
    OpaqueWithRegisteredBase(QObject *parent = nullptr) : RegisteredBase(parent) {}
private:
    int m_w = 4;
};

// Not registered: it is the base of both an opaque and a non-opaque type (see
// below). Because one of its derived types is registered, it must be registered
// normally and must not end up as opaque.
class SharedBase : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int s MEMBER m_s CONSTANT)
public:
    SharedBase(QObject *parent = nullptr) : QObject(parent) {}
private:
    int m_s = 5;
};

// Not registered: only used as a property type, so it becomes opaque. As an
// opaque type it would normally drag in its base as opaque too, but SharedBase
// is also the base of a registered type, so it must stay registered.
class OpaqueSharingBase : public SharedBase
{
    Q_OBJECT
    Q_PROPERTY(int os MEMBER m_os CONSTANT)
public:
    OpaqueSharingBase(QObject *parent = nullptr) : SharedBase(parent) {}
private:
    int m_os = 6;
};

// Registered type deriving from SharedBase. This is what keeps SharedBase from
// becoming opaque.
class RegisteredSharingBase : public SharedBase
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int rs MEMBER m_rs CONSTANT)
public:
    RegisteredSharingBase(QObject *parent = nullptr) : SharedBase(parent) {}
private:
    int m_rs = 7;
};

// Registered holder that references the opaque types as properties, forcing
// them to be collected as opaque types.
class OpaqueHolder : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(OpaqueWithOpaqueBase *withOpaqueBase MEMBER m_a CONSTANT)
    Q_PROPERTY(OpaqueWithRegisteredBase *withRegisteredBase MEMBER m_b CONSTANT)
    Q_PROPERTY(OpaqueSharingBase *sharing MEMBER m_c CONSTANT)
public:
    OpaqueHolder(QObject *parent = nullptr) : QObject(parent) {}
private:
    OpaqueWithOpaqueBase *m_a = nullptr;
    OpaqueWithRegisteredBase *m_b = nullptr;
    OpaqueSharingBase *m_c = nullptr;
};

#endif // OPAQUETYPES_H
