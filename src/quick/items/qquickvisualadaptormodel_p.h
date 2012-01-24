/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKVISUALADAPTORMODEL_P_H
#define QQUICKVISUALADAPTORMODEL_P_H

#include <QtCore/qobject.h>
#include <QtCore/qabstractitemmodel.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QDeclarativeEngine;

class QQuickVisualDataModelItem;
class QQuickVisualDataModelItemMetaType;

class QQuickVisualAdaptorModelPrivate;
class QQuickVisualAdaptorModel : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QQuickVisualAdaptorModel)
public:
    enum Flag
    {
        ProxiedObject       = 0x01
    };
    Q_DECLARE_FLAGS(Flags, Flag)

    QQuickVisualAdaptorModel(QObject *parent = 0);
    virtual ~QQuickVisualAdaptorModel();

    Flags flags() const;

    QVariant model() const;
    void setModel(const QVariant &, QDeclarativeEngine *);

    QVariant rootIndex() const;
    void setRootIndex(const QVariant &root);

    QVariant modelIndex(int idx) const;
    QVariant parentModelIndex() const;

    int count() const;
    QQuickVisualDataModelItem *createItem(QQuickVisualDataModelItemMetaType *metaType, int index);
    QString stringValue(int index, const QString &role);
    void replaceWatchedRoles(const QList<QByteArray> &oldRoles, const QList<QByteArray> &newRoles);

    bool canFetchMore() const;
    void fetchMore();

Q_SIGNALS:
    void rootIndexChanged();
    void modelReset(int oldCount, int newCount);

    void itemsInserted(int index, int count);
    void itemsRemoved(int index, int count);
    void itemsMoved(int from, int to, int count);
    void itemsChanged(int index, int count);

private Q_SLOTS:
    void _q_itemsChanged(int, int, const QList<int> &);
    void _q_itemsInserted(int index, int count);
    void _q_itemsRemoved(int index, int count);
    void _q_itemsMoved(int from, int to, int count);
    void _q_rowsInserted(const QModelIndex &,int,int);
    void _q_rowsRemoved(const QModelIndex &,int,int);
    void _q_rowsMoved(const QModelIndex &, int, int, const QModelIndex &, int);
    void _q_dataChanged(const QModelIndex&,const QModelIndex&);
    void _q_layoutChanged();
    void _q_modelReset();

private:
    Q_DISABLE_COPY(QQuickVisualAdaptorModel)
};

class QQuickVisualAdaptorModelProxyInterface
{
public:
    virtual ~QQuickVisualAdaptorModelProxyInterface() {}

    virtual QObject *proxiedObject() = 0;
};

Q_DECLARE_INTERFACE(QQuickVisualAdaptorModelProxyInterface, "com.trolltech.qml.QQuickVisualAdaptorModelProxyInterface")

QT_END_NAMESPACE

#endif
