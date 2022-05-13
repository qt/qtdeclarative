// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmltypereader.h"

#include <QFileInfo>
#include <QFile>
#include <QRegularExpression>

#include <iostream>

QStringList readQmlTypes(const QString &filename) {
    QRegularExpression re("import QtQuick\\.tooling 1\\.2.*Module {\\s*dependencies:\\s*\\[([^\\]]*)\\](.*)}",
                          QRegularExpression::DotMatchesEverythingOption);
    if (!QFileInfo(filename).exists()) {
        std::cerr << "Non existing file: " << filename.toStdString() << std::endl;
        return QStringList();
    }
    QFile f(filename);
    if (!f.open(QFileDevice::ReadOnly)) {
        std::cerr << "Error in opening file " << filename.toStdString() << " : "
                  << f.errorString().toStdString() << std::endl;
        return QStringList();
    }
    QByteArray fileData = f.readAll();
    QString data(fileData);
    QRegularExpressionMatch m = re.match(data);
    if (m.lastCapturedIndex() != 2) {
        std::cerr << "Malformed file: " << filename.toStdString() << std::endl;
        return QStringList();
    }
    return m.capturedTexts();
}
