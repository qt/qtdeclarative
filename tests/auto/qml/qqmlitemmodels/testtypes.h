/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef TESTTYPES_H
#define TESTTYPES_H

#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qitemselectionmodel.h>
#include "qdebug.h"

class ItemModelsTest : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QAbstractItemModel *model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(QModelIndex modelIndex READ modelIndex WRITE setModelIndex NOTIFY changed)
    Q_PROPERTY(QPersistentModelIndex persistentModelIndex READ persistentModelIndex WRITE setPersistentModelIndex NOTIFY changed)

public:
    ItemModelsTest()
        : m_model(0)
    {}

    QModelIndex modelIndex() const
    {
        return m_modelIndex;
    }

    QPersistentModelIndex persistentModelIndex() const
    {
        return m_persistentModelIndex;
    }

    void emitChanged()
    {
        emit changed();
    }

    void emitSignalWithModelIndex(const QModelIndex &index)
    {
        emit signalWithModelIndex(index);
    }

    void emitSignalWithPersistentModelIndex(const QPersistentModelIndex &index)
    {
        emit signalWithPersistentModelIndex(index);
    }

    QAbstractItemModel * model() const
    {
        return m_model;
    }

    Q_INVOKABLE QModelIndex invalidModelIndex() const
    {
        return QModelIndex();
    }

    Q_INVOKABLE QModelIndexList createModelIndexList() const
    {
        return QModelIndexList();
    }

    Q_INVOKABLE QItemSelectionRange createItemSelectionRange(const QModelIndex &tl, const QModelIndex &br) const
    {
        return QItemSelectionRange(tl, br);
    }

    Q_INVOKABLE QItemSelection createItemSelection()
    {
        return QItemSelection();
    }

    Q_INVOKABLE QPersistentModelIndex createPersistentModelIndex(const QModelIndex &index)
    {
        return QPersistentModelIndex(index);
    }

public slots:
    void setModelIndex(const QModelIndex &arg)
    {
        if (m_modelIndex == arg)
            return;

        m_modelIndex = arg;
        emit changed();
    }

    void setPersistentModelIndex(const QPersistentModelIndex &arg)
    {
        if (m_persistentModelIndex == arg)
            return;

        m_persistentModelIndex = arg;
        emit changed();
    }

    void setModel(QAbstractItemModel *arg)
    {
        if (m_model == arg)
            return;

        m_model = arg;
        emit modelChanged(arg);
    }

signals:
    void changed();

    void signalWithModelIndex(QModelIndex index);
    void signalWithPersistentModelIndex(QPersistentModelIndex index);

    void modelChanged(QAbstractItemModel * arg);

private:
    QModelIndex m_modelIndex;
    QPersistentModelIndex m_persistentModelIndex;
    QAbstractItemModel *m_model;
};

#endif // TESTTYPES_H

