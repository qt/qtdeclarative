// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef TYPEWITHNAMESPACE_H
#define TYPEWITHNAMESPACE_H

#include <QtCore/qobject.h>
#include <QtQml/qqmlregistration.h>

namespace MyNamespace {

class TypeWithNamespace : public QObject
{
    Q_OBJECT
    QML_ELEMENT
public:
    TypeWithNamespace(QObject *parent = nullptr);

signals:
};

namespace Sub1 {
namespace Sub2 {
namespace Sub3 {

class TypeWithSubnamespace : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    int m_x = 55;

public:
    TypeWithSubnamespace(QObject *parent = nullptr);

    Q_PROPERTY(int x READ x WRITE setX NOTIFY xChanged);
    int x() { return m_x; }
    void setX(int x)
    {
        if (x == m_x)
            return;
        m_x = x;
        emit xChanged();
    }

signals:
    void xChanged();
};
} // namespace Sub3
} // namespace Sub2
} // namespace Sub1
} // namespace MyNamespace

#endif // TYPEWITHNAMESPACE_H
