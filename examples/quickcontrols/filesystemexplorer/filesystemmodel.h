// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef FILESYSTEMMODEL_H
#define FILESYSTEMMODEL_H

#include <QFileSystemModel>
#include <QQuickTextDocument>

class FileSystemModel : public QFileSystemModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(QModelIndex rootIndex READ rootIndex WRITE setRootIndex NOTIFY rootIndexChanged)
public:
    explicit FileSystemModel(QObject *parent = nullptr);

    // Functions invokable from QML
    Q_INVOKABLE QString readFile(const QString &filePath);
    Q_INVOKABLE int currentLineNumber(QQuickTextDocument *textDocument, int cursorPosition);

    // Overridden functions
    int columnCount(const QModelIndex &parent) const override;

    // Member functions from here
    QModelIndex rootIndex() const;
    void setRootIndex(const QModelIndex index);
    void setInitialDirectory(const QString &path = getDefaultRootDir());

    static QString getDefaultRootDir();

signals:
    void rootIndexChanged();

private:
    QModelIndex m_rootIndex;
};

#endif
