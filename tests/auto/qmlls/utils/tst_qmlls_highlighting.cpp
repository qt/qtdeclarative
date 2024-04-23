// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qmlls_highlighting.h"

#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQmlDom/private/qqmldomitem_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>
#include <QtQmlLS/private/qqmlsemantictokens_p.h>

#include <QtLanguageServer/private/qlanguageserverspectypes_p.h>

#include <qlist.h>

using namespace QLspSpecification;

tst_qmlls_highlighting::tst_qmlls_highlighting()
    : QQmlDataTest(QT_QMLLS_HIGHLIGHTS_DATADIR) , m_highlightingDataDir(QT_QMLLS_HIGHLIGHTS_DATADIR + "/highlights"_L1)
{
}

// Token encoding as in:
// https://microsoft.github.io/language-server-protocol/specifications/specification-3-16/#textDocument_semanticTokens
void tst_qmlls_highlighting::encodeSemanticTokens_data()
{
    QTest::addColumn<Highlights>("highlights");
    QTest::addColumn<QList<int>>("expectedMemoryLayout");

    {
        Highlights c;
        c.highlights().insert(0, Token());
        QTest::addRow("empty-token-single") << c << QList {0, 0, 0, 0, 0};
    }
    {
        Highlights c;
        QQmlJS::SourceLocation loc(0, 1, 1, 1);
        c.highlights().insert(0, Token(loc, 0, 0));
        QTest::addRow("single-token") << c << QList {0, 0, 1, 0, 0};
    }
    {
        Highlights c;
        Token t1(QQmlJS::SourceLocation(0, 1, 1, 1), 0, 0);
        Token t2(QQmlJS::SourceLocation(1, 1, 3, 3), 0, 0);
        c.highlights().insert(t1.offset, t1);
        c.highlights().insert(t2.offset, t2);
        QTest::addRow("different-lines") << c << QList {0, 0, 1, 0, 0, 2, 2, 1, 0, 0};
    }
    {
        Highlights c;
        Token t1(QQmlJS::SourceLocation(0, 1, 1, 1), 0, 0);
        Token t2(QQmlJS::SourceLocation(1, 1, 1, 3), 0, 0);
        c.highlights().insert(t1.offset, t1);
        c.highlights().insert(t2.offset, t2);
        QTest::addRow("same-line-different-column") << c << QList {0, 0, 1, 0, 0, 0, 2, 1, 0, 0};
    }
    {
        Highlights c;
        Token t1(QQmlJS::SourceLocation(0, 1, 1, 1), 1, 0);
        c.highlights().insert(t1.offset, t1);
        QTest::addRow("token-type") << c << QList {0, 0, 1, 1, 0};
    }
    {
        Highlights c;
        Token t1(QQmlJS::SourceLocation(0, 1, 1, 1), 1, 1);
        c.highlights().insert(t1.offset, t1);
        QTest::addRow("token-modifier") << c << QList {0, 0, 1, 1, 1};
    }
}

void tst_qmlls_highlighting::encodeSemanticTokens()
{
    QFETCH(Highlights, highlights);
    QFETCH(QList<int>, expectedMemoryLayout);
    const auto encoded = HighlightingUtils::encodeSemanticTokens(highlights);
    QCOMPARE(encoded, expectedMemoryLayout);
}

QTEST_MAIN(tst_qmlls_highlighting)
