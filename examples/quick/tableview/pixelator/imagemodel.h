// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef IMAGEMODEL_H
#define IMAGEMODEL_H

#include <QAbstractTableModel>
#include <QImage>
#include <QtQml/qqml.h>

//! [model]
class ImageModel : public QAbstractTableModel
{
    Q_OBJECT
    Q_PROPERTY(QString source READ source WRITE setSource NOTIFY sourceChanged)
    QML_ELEMENT
public:
    ImageModel(QObject *parent = nullptr);

    QString source() const;
    void setSource(const QString &source);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QVariant headerData(int /* section */, Qt::Orientation /* orientation */,
                        int role) const override;

signals:
    void sourceChanged();

private:
    QString m_source;
    QImage m_image;
};
//! [model]

#endif // IMAGEMODEL_H
