// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QTBRIDGEREADER_H
#define QTBRIDGEREADER_H

#include <QtCore>
#include <QJsonDocument>
#include <QtGui/private/qzipreader_p.h>

#include <stdexcept>

class QtBridgeReader
{
public:
    QtBridgeReader(const QString &bridgeFile)
    {
        m_destDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
        + "/" + qApp->applicationName();

        QZipReader zip(bridgeFile);
        if (!zip.isReadable())
            throw std::runtime_error("Could not read input file: " + bridgeFile.toStdString());
        if (!QDir().mkpath(m_destDir))
            throw std::runtime_error("Could not create tmp path: " + m_destDir.toStdString());
        if (!zip.extractAll(m_destDir))
            throw std::runtime_error("Could not unzip input file: " + std::to_string(zip.status()));

        const auto fileList = QDir(m_destDir).entryList(QStringList("*.metadata"));
        if (fileList.isEmpty())
            throw std::runtime_error("Could not fine a .metadata inside the input file!");

        const QString metaDataFileName = m_destDir + "/" + fileList.first();
        QFile metaDataFile(metaDataFileName);
        if (!metaDataFile.exists())
            throw std::runtime_error("File doesn't exist: " + metaDataFileName.toStdString());
        if (!metaDataFile.open(QFile::ReadOnly))
            throw std::runtime_error("Could not open file for reading: " + metaDataFileName.toStdString());

        QJsonParseError error;
        m_document = QJsonDocument::fromJson(metaDataFile.readAll(), &error);
        if (m_document.isNull())
            throw std::runtime_error("Could not parse json file: " + error.errorString().toStdString());
    }

    ~QtBridgeReader()
    {
        QDir(m_destDir).removeRecursively();
    }

    QJsonDocument document() const
    {
        return m_document;
    }

    QString resourcePath() const
    {
        return m_destDir;
    }

    QStringList entryList()
    {
        return QDir(m_destDir).entryList();
    }

private:
    QString m_destDir;
    QJsonDocument m_document;
};

#endif
