// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TST_QMLLS_DOCUMENTATION_H
#define TST_QMLLS_DOCUMENTATION_H

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtCore/qobject.h>
#include <QtTest/qtest.h>

class tst_qmlls_documentationHints : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qmlls_documentationHints();
private slots:
    void qdochtmlparser_data();
    void qdochtmlparser();

    void skipParsingHtmlTags_data();
    void skipParsingHtmlTags();
private:
    QString m_documentationDataDir;
};

#endif // TST_QMLLS_DOCUMENTATION_H
