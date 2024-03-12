// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef INVISIBLE_H
#define INVISIBLE_H

#include <QtCore/qobject.h>
#include <QtQmlIntegration/qqmlintegration.h>
#include <QtQml/qqmllist.h>

class Invisible : public QObject
{
public:
    Invisible(QObject *parent = nullptr) : QObject(parent) {}
};

class SingletonModel : public Invisible
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    SingletonModel(QObject *parent = nullptr) : Invisible(parent) {}
};

class AttachedAttached : public Invisible
{
    Q_OBJECT
public:
    AttachedAttached(QObject *parent = nullptr) : Invisible(parent) {}
};

class AttachedObject : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_ATTACHED(AttachedAttached)
public:
    static AttachedAttached *qmlAttachedProperties(QObject *object)
    {
        return new AttachedAttached(object);
    }
};

class DerivedFromInvisible : public Invisible
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(double implicitWidth MEMBER m_implicitWidth NOTIFY implicitWidthChanged FINAL)
public:
    DerivedFromInvisible(QObject *parent = nullptr) : Invisible(parent) {}

signals:
    void implicitWidthChanged();

private:
    double m_implicitWidth = 27;
};

class WithListPropertyOfDerivedFromInvisible : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QQmlListProperty<DerivedFromInvisible> children READ children NOTIFY childrenChanged FINAL)

public:
    WithListPropertyOfDerivedFromInvisible(QObject *parent = nullptr) : QObject(parent)
    {
        m_children.append(new DerivedFromInvisible(this));
    }

    QQmlListProperty<DerivedFromInvisible> children()
    {
        return QQmlListProperty<DerivedFromInvisible>(this, &m_children);
    }

signals:
    void childrenChanged();

private:
    QList<DerivedFromInvisible *> m_children;
};

#endif // INVISIBLE_H
