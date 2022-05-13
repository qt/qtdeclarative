// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    Q_PROPERTY(QModelIndexList modelIndexList READ modelIndexList WRITE setModelIndexList NOTIFY changed)
    Q_PROPERTY(QItemSelection itemSelection READ itemSelection WRITE setItemSelection NOTIFY changed)

public:
    ItemModelsTest(QObject *parent = nullptr)
        : QObject(parent)
        , m_model(0)
    {
    }

    QModelIndex modelIndex() const
    {
        return m_modelIndex;
    }

    QPersistentModelIndex persistentModelIndex() const
    {
        return m_persistentModelIndex;
    }

    QModelIndexList modelIndexList()
    {
        static bool firstTime = true;
        if (firstTime && m_model && m_modelIndexList.isEmpty()) {
            firstTime = false;
            for (int i = 0; i < m_model->rowCount(); i++)
                m_modelIndexList << m_model->index(i, 0);
        }
        return m_modelIndexList;
    }

    Q_INVOKABLE QModelIndexList someModelIndexList() const
    {
        QModelIndexList list;
        if (m_model)
            for (int i = 0; i < m_model->rowCount(); i++)
                list << m_model->index(i, 0);
        return list;
    }

    QItemSelection itemSelection() const
    {
        return m_itemSelection;
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

    void setModelIndexList(QModelIndexList arg)
    {
        if (m_modelIndexList == arg)
            return;

        m_modelIndexList = arg;
        emit changed();
    }

    void setItemSelection(QItemSelection arg)
    {
        if (m_itemSelection == arg)
            return;

        m_itemSelection = arg;
        emit changed();
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
    QModelIndexList m_modelIndexList;
    QItemSelection m_itemSelection;
};

#endif // TESTTYPES_H

