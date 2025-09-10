// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLPREVIEWFILEENGINE_H
#define QQMLPREVIEWFILEENGINE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qqmlpreviewfileloader.h"

#include <QtCore/qpointer.h>
#include <private/qabstractfileengine_p.h>
#include <private/qfsfileengine_p.h>
#include <QtCore/qbuffer.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QQmlPreviewFileEngine : public QAbstractFileEngine
{
public:
    QQmlPreviewFileEngine(const QString &file, const QString &absolute,
                          QQmlPreviewFileLoader *loader);

    void setFileName(const QString &file) override;

    bool open(QIODevice::OpenMode flags, std::optional<QFile::Permissions> permissions) override;
    bool close() override;
    qint64 size() const override;
    qint64 pos() const override;
    bool seek(qint64) override;
    qint64 read(char *data, qint64 maxlen) override;

    FileFlags fileFlags(FileFlags type) const override;
    QString fileName(QAbstractFileEngine::FileName file) const override;
    uint ownerId(FileOwner) const override;

    IteratorUniquePtr beginEntryList(const QString &path, QDirListing::IteratorFlags filters,
                                     const QStringList &filterNames) override;
    IteratorUniquePtr endEntryList() override;

    // Forwarding to fallback if exists
    bool flush() override;
    bool syncToDisk() override;
    bool isSequential() const override;
    bool remove() override;
    bool copy(const QString &newName) override;
    bool rename(const QString &newName) override;
    bool renameOverwrite(const QString &newName) override;
    bool link(const QString &newName) override;
    bool mkdir(const QString &dirName, bool createParentDirectories,
               std::optional<QFile::Permissions> permissions = std::nullopt) const override;
    bool rmdir(const QString &dirName, bool recurseParentDirectories) const override;
    bool setSize(qint64 size) override;
    bool caseSensitive() const override;
    bool isRelativePath() const override;
    QStringList entryList(QDir::Filters filters, const QStringList &filterNames) const override;
    bool setPermissions(uint perms) override;
    QByteArray id() const override;
    QString owner(FileOwner) const override;
    QDateTime fileTime(QFile::FileTime time) const override;
    int handle() const override;
    qint64 readLine(char *data, qint64 maxlen) override;
    qint64 write(const char *data, qint64 len) override;
    bool extension(Extension extension, const ExtensionOption *option, ExtensionReturn *output) override;
    bool supportsExtension(Extension extension) const override;

private:
    void load() const;

    QString m_name;
    QString m_absolute;
    QPointer<QQmlPreviewFileLoader> m_loader;

    mutable QBuffer m_contents;
    mutable QStringList m_entries;
    mutable std::unique_ptr<QAbstractFileEngine> m_fallback;
    mutable QQmlPreviewFileLoader::Result m_result = QQmlPreviewFileLoader::Unknown;
};

class QQmlPreviewFileEngineHandler : public QAbstractFileEngineHandler
{
    Q_DISABLE_COPY_MOVE(QQmlPreviewFileEngineHandler)
public:
    QQmlPreviewFileEngineHandler(QQmlPreviewFileLoader *loader);
    std::unique_ptr<QAbstractFileEngine> create(const QString &fileName) const override;

private:
    QPointer<QQmlPreviewFileLoader> m_loader;
};



QT_END_NAMESPACE

#endif // QQMLPREVIEWFILEENGINE_H
