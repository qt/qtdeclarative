// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "filesystemmodel.h"

#include <QStandardPaths>
#include <QMimeDatabase>
#include <QTextDocument>
#include <QTextObject>

FileSystemModel::FileSystemModel(QObject *parent) : QFileSystemModel(parent)
{
    setFilter(QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot);
    setInitialDirectory();
}

QString FileSystemModel::readFile(const QString &filePath)
{
    // Don't issue errors for an empty path, as the initial binding
    // will result in an empty path, and that's OK.
    if (filePath.isEmpty())
        return {};

    QFile file(filePath);

    if (file.size() >= 2'000'000)
        return tr("File size is too big.\nYou can read files up to %1 MB.").arg(2);

    static const QMimeDatabase db;
    const QMimeType mime = db.mimeTypeForFile(QFileInfo(file));

    // Check if the mimetype is supported and return the content.
    const auto mimeTypesForFile = mime.parentMimeTypes();
    for (const auto &m : mimeTypesForFile) {
        if (m.contains("text", Qt::CaseInsensitive)
                || mime.comment().contains("text", Qt::CaseInsensitive)) {
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
                return tr("Error opening the File!");

            QTextStream stream(&file);
            return stream.readAll();
        }
    }
    return tr("Filetype not supported!");
}

// This function gets called from Editor.qml
int FileSystemModel::currentLineNumber(QQuickTextDocument *textDocument, int cursorPosition)
{
    if (QTextDocument *td = textDocument->textDocument()) {
        QTextBlock tb = td->findBlock(cursorPosition);
        return tb.blockNumber();
    }
    return -1;
}

int FileSystemModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return 1;
}

QModelIndex FileSystemModel::rootIndex() const
{
    return m_rootIndex;
}

void FileSystemModel::setRootIndex(const QModelIndex index)
{
    if (index == m_rootIndex)
        return;
    m_rootIndex = index;
    emit rootIndexChanged();
}

void FileSystemModel::setInitialDirectory(const QString &path)
{
    QDir dir(path);
    if (dir.makeAbsolute())
        setRootPath(dir.path());
    else
        setRootPath(getDefaultRootDir());
    setRootIndex(QFileSystemModel::index(dir.path(), 0));
}

QString FileSystemModel::getDefaultRootDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
}
