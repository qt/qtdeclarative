/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
****************************************************************************/
#ifndef RESOURCEFILEMAPPER_H
#define RESOURCEFILEMAPPER_H

#include <QStringList>
#include <QHash>
#include <QFile>

struct ResourceFileMapper
{
    enum class FileOutput {
        RelativeFilePath,
        AbsoluteFilePath
    };
    ResourceFileMapper(const QStringList &resourceFiles);

    bool isEmpty() const;

    QStringList resourcePaths(const QString &fileName);
    QStringList qmlCompilerFiles(FileOutput fo = FileOutput::RelativeFilePath) const;

private:
    void populateFromQrcFile(QFile &file);

    QHash<QString, QString> qrcPathToFileSystemPath;
};

#endif // RESOURCEFILEMAPPER_H
