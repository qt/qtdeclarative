// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef TST_QMLLS_QQMLCODEMODEL_H
#define TST_QMLLS_QQMLCODEMODEL_H

#include <QtJsonRpc/private/qjsonrpcprotocol_p.h>
#include <QtLanguageServer/private/qlanguageserverprotocol_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <QtCore/qobject.h>
#include <QtCore/qprocess.h>
#include <QtCore/qlibraryinfo.h>

#include <QtTest/qtest.h>

#include <iostream>

using namespace Qt::StringLiterals;

class tst_qmlls_qqmlcodemodel : public QObject
{
    Q_OBJECT

private slots:
    void buildPathsForFileUrl_data();
    void buildPathsForFileUrl();
};

#endif // TST_QMLLS_QQMLCODEMODEL_H
