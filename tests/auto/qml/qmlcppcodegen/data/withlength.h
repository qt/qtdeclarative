// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef WITHLENGTH_H
#define WITHLENGTH_H

#include <QtCore/qobject.h>
#include <QtCore/qpoint.h>
#include <QtCore/qrect.h>
#include <QtQml/qqml.h>

class ObjectType : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    ObjectType(QObject *parent = nullptr) : QObject(parent) {}
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
    Q_INVOKABLE ValueTypeWithLength(QPointF point) : m_length(point.manhattanLength()) {}
    Q_INVOKABLE ValueTypeWithLength(QRectF rect) : m_length(rect.width()) {}
    Q_INVOKABLE ValueTypeWithLength(ObjectType *o) : m_length(o->objectName().length()) {}
    Q_INVOKABLE QString toString() const { return QStringLiteral("no"); }

    int length() const { return m_length; }

private:
    int m_length = 19;
};

struct InnerWithLength {
    int m_length;
};

struct UnconstructibleWithLength
{
    Q_GADGET
    QML_VALUE_TYPE(unconstructibleWithLength)

    QML_FOREIGN(InnerWithLength)
    QML_EXTENDED(UnconstructibleWithLength)

public:
    UnconstructibleWithLength() = default;
    Q_INVOKABLE UnconstructibleWithLength(int length) : v{length} {}

private:
    InnerWithLength v;
};

#endif // WITHLENGTH_H
