/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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
