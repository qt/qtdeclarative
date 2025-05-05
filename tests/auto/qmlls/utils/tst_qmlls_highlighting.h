// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TST_QMLLS_HIGHLIGHTING_H
#define TST_QMLLS_HIGHLIGHTING_H

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtCore/qobject.h>
#include <QtTest/qtest.h>

class tst_qmlls_highlighting : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qmlls_highlighting();
private slots:
    void encodeSemanticTokens_data();
    void encodeSemanticTokens();
    void sourceLocationsFromMultiLineToken_data();
    void sourceLocationsFromMultiLineToken();

    void highlights_data();
    void highlights();

    void rangeOverlapsWithSourceLocation_data();
    void rangeOverlapsWithSourceLocation();

    void updateResultID_data();
    void updateResultID();

    void computeDiff_data();
    void computeDiff();

    void enumCrash();

private:
    QString m_highlightingDataDir;
};

#endif // TST_QMLLS_HIGHLIGHTING_H
