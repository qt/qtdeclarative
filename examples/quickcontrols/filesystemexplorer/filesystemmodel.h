// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef FILESYSTEMMODEL_H
#define FILESYSTEMMODEL_H

#include <QFileSystemModel>
#include <QtQml/qqml.h>

class FileSystemModel : public QFileSystemModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit FileSystemModel(QObject *parent = nullptr);
    int columnCount(const QModelIndex &parent) const override;
    Q_INVOKABLE QString readFile(const QString &filePath);
};

#endif
