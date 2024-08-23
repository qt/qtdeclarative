// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef ERRORLISTMODEL_H
#define ERRORLISTMODEL_H

#include <QAbstractListModel>

class QQmlError;

class ErrorListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit ErrorListModel(QObject *parent = nullptr);

    void setErrorList(const QList<QQmlError> &errorList);

private:
    bool isIndexValid(const QModelIndex &index) const;

    // QAbstractItemModel interface
    int rowCount(const QModelIndex &parent = QModelIndex{}) const final;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const final;

signals:
    void errorPositionSelected(int line, int column);

public slots:
    void selectIndex(const QModelIndex &index);

private:
    QList<QQmlError> m_errorList;
};

#endif // ERRORLISTMODEL_H
