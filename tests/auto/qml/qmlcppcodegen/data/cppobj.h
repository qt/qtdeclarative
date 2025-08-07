// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef CPPOBJ_H
#define CPPOBJ_H

#include <QtCore/qobject.h>
#include <QtCore/qdebug.h>
#include <QtQmlIntegration/qqmlintegration.h>

class CppObj : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int identifier MEMBER m_identifier CONSTANT)

public:
    explicit CppObj(int identifier = -1, QObject *parent = nullptr)
        : QObject(parent)
        , m_identifier(identifier)
    {
        qDebug() << "Object created:" << m_identifier;
    }

    ~CppObj() { qDebug() << "Object destroyed:" << m_identifier; }

private:
    int m_identifier = -1;
};

class CppBridge : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit CppBridge(QObject *parent = nullptr) : QObject{parent} {}

    Q_INVOKABLE CppObj *singleObj(int identifier) const
    {
        return new CppObj(identifier);
    }
};

#endif // CPPOBJ_H
