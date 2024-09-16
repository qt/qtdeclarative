// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef GADGETMODEL_H
#define GADGETMODEL_H

#include <QAbstractItemModel>
#include <QObject>
#include <qqml.h>

class MyGadget
{
    Q_GADGET
    Q_PROPERTY(QString name MEMBER m_name CONSTANT)
    Q_PROPERTY(QString size MEMBER m_size CONSTANT)

public:
    MyGadget(QString name, QString size);

private:
    QString m_name;
    QString m_size;
};

class GadgetModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
public:
    explicit GadgetModel(QObject* parent = nullptr);

    Q_INVOKABLE int rowCount(
            const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index,
                  int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    QVector<MyGadget> m_gadgets;
};

#endif // GADGETMODEL_H
