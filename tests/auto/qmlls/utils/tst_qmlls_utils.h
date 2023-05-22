// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef TST_QMLLS_UTILS_H
#define TST_QMLLS_UTILS_H

#include <QtJsonRpc/private/qjsonrpcprotocol_p.h>
#include <QtLanguageServer/private/qlanguageserverprotocol_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <QtCore/qobject.h>
#include <QtCore/qprocess.h>
#include <QtCore/qlibraryinfo.h>

#include <QtTest/qtest.h>

#include <QtQmlLS/private/qqmllsutils_p.h>

#include <iostream>

using namespace Qt::StringLiterals;

class tst_qmlls_utils : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_qmlls_utils() : QQmlDataTest(QT_QMLLS_UTILS_DATADIR) { }

private slots:
    void textOffsetRowColumnConversions_data();
    void textOffsetRowColumnConversions();

    void findItemFromLocation_data();
    void findItemFromLocation();

    void findTypeDefinitionFromLocation_data();
    void findTypeDefinitionFromLocation();

    void findDefinitionFromLocation_data();
    void findDefinitionFromLocation();

    void findLocationOfItem_data();
    void findLocationOfItem();

    void findBaseObject();
    void findBaseObject_data();

    void findUsages();
    void findUsages_data();

private:
    using EnvironmentAndFile = std::tuple<QQmlJS::Dom::DomItem, QQmlJS::Dom::DomItem>;

    EnvironmentAndFile createEnvironmentAndLoadFile(const QString &file,
                                                    QQmlJS::Dom::DomCreationOptions options);

    using CacheKey = std::pair<QString, QQmlJS::Dom::DomCreationOptions>;
    // avoid loading the same file over and over when running all the tests
    QHash<CacheKey, EnvironmentAndFile> cache;
};

#endif // TST_QMLLS_UTILS_H
