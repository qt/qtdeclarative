// Commit: ac5c099cc3c5b8c7eec7a49fdeb8a21037230350
/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSGVISUALITEMMODEL_P_H
#define QSGVISUALITEMMODEL_P_H

#include <QtDeclarative/qdeclarative.h>
#include <QtCore/qobject.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QSGItem;

class Q_DECLARATIVE_EXPORT QSGVisualModel : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    virtual ~QSGVisualModel() {}

    enum ReleaseFlag { Referenced = 0x01, Destroyed = 0x02 };
    Q_DECLARE_FLAGS(ReleaseFlags, ReleaseFlag)

    virtual int count() const = 0;
    virtual bool isValid() const = 0;
    virtual QSGItem *item(int index, bool complete=true) = 0;
    virtual ReleaseFlags release(QSGItem *item) = 0;
    virtual bool completePending() const = 0;
    virtual void completeItem() = 0;
    virtual QString stringValue(int, const QString &) = 0;
    virtual void setWatchedRoles(QList<QByteArray> roles) = 0;

    virtual int indexOf(QSGItem *item, QObject *objectContext) const = 0;

Q_SIGNALS:
    void countChanged();
    void itemsInserted(int index, int count);
    void itemsRemoved(int index, int count);
    void itemsMoved(int from, int to, int count);
    void itemsChanged(int index, int count);
    void modelReset();
    void createdItem(int index, QSGItem *item);
    void destroyingItem(QSGItem *item);

protected:
    QSGVisualModel(QObjectPrivate &dd, QObject *parent = 0)
        : QObject(dd, parent) {}

private:
    Q_DISABLE_COPY(QSGVisualModel)
};

class QSGVisualItemModelAttached;
class QSGVisualItemModelPrivate;
class Q_DECLARATIVE_EXPORT QSGVisualItemModel : public QSGVisualModel
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QSGVisualItemModel)

    Q_PROPERTY(QDeclarativeListProperty<QSGItem> children READ children NOTIFY childrenChanged DESIGNABLE false)
    Q_CLASSINFO("DefaultProperty", "children")

public:
    QSGVisualItemModel(QObject *parent=0);
    virtual ~QSGVisualItemModel() {}

    virtual int count() const;
    virtual bool isValid() const;
    virtual QSGItem *item(int index, bool complete=true);
    virtual ReleaseFlags release(QSGItem *item);
    virtual bool completePending() const;
    virtual void completeItem();
    virtual QString stringValue(int index, const QString &role);
    virtual void setWatchedRoles(QList<QByteArray>) {}

    virtual int indexOf(QSGItem *item, QObject *objectContext) const;

    QDeclarativeListProperty<QSGItem> children();

    static QSGVisualItemModelAttached *qmlAttachedProperties(QObject *obj);

Q_SIGNALS:
    void childrenChanged();

private:
    Q_DISABLE_COPY(QSGVisualItemModel)
};

class QSGVisualItemModelAttached : public QObject
{
    Q_OBJECT

public:
    QSGVisualItemModelAttached(QObject *parent)
        : QObject(parent), m_index(0) {}
    ~QSGVisualItemModelAttached() {
        attachedProperties.remove(parent());
    }

    Q_PROPERTY(int index READ index NOTIFY indexChanged)
    int index() const { return m_index; }
    void setIndex(int idx) {
        if (m_index != idx) {
            m_index = idx;
            emit indexChanged();
        }
    }

    static QSGVisualItemModelAttached *properties(QObject *obj) {
        QSGVisualItemModelAttached *rv = attachedProperties.value(obj);
        if (!rv) {
            rv = new QSGVisualItemModelAttached(obj);
            attachedProperties.insert(obj, rv);
        }
        return rv;
    }

Q_SIGNALS:
    void indexChanged();

public:
    int m_index;

    static QHash<QObject*, QSGVisualItemModelAttached*> attachedProperties;
};


QT_END_NAMESPACE

QML_DECLARE_TYPE(QSGVisualModel)
QML_DECLARE_TYPE(QSGVisualItemModel)
QML_DECLARE_TYPEINFO(QSGVisualItemModel, QML_HAS_ATTACHED_PROPERTIES)

QT_END_HEADER

#endif // QSGVISUALITEMMODEL_P_H
