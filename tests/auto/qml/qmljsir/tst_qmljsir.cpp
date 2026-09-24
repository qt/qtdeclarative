// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tst_qmljsir.h"

#include <QtQml/private/qqmlirbuilder_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

using namespace QmlIR;
using namespace Syntax;

void tst_qmljsir::failureMode()
{
    const QString source = QStringLiteral("QtObject { signal Array() }");

    Document stopDocument(QString(), QString(), true);
    IRBuilder stopBuilder;
    QVERIFY(!stopBuilder.generateFromQml(source, QString(), &stopDocument));
    QCOMPARE(stopBuilder.errors.size(), 1);

    Document continueDocument(QString(), QString(), true);
    IRBuilder continueBuilder{ IRBuilder::ContinueAfterFailure };
    QVERIFY(!continueBuilder.generateFromQml(source, QString(), &continueDocument));
    QCOMPARE(continueBuilder.errors.size(), 2);
}

void tst_qmljsir::propertyVirtSpecifiers_data()
{
    QTest::addColumn<Phrase>("virtSpecifiers");
    QTest::addColumn<Property>("propertyWithVirtFlags");

    const auto addTestRow = [](const Phrase &virtSpecifiers,
                               const Property &propertyWithVirtFlags) {
        QTest::addRow("%s", qPrintable(toString(virtSpecifiers)))
                << virtSpecifiers << propertyWithVirtFlags;
    };
    addTestRow({}, {});
    {
        Property p{};
        p.setIsVirtual(true);
        addTestRow({ Token::T_VIRTUAL }, p);
    }
    {
        Property p{};
        p.setIsOverride(true);
        addTestRow({ Token::T_OVERRIDE }, p);
    }
    {
        Property p{};
        p.setIsFinal(true);
        addTestRow({ Token::T_FINAL }, p);
    }
}

// ideally, return type would be optional<Document>, or some IRConstructionResult,
// but Copy&Move are disabled on the Document because of the MemoryPool in JsEngine
static auto tryBuildIRDocumentFrom(const QString &source) -> std::unique_ptr<Document>
{
    auto documentPtr = std::make_unique<Document>(QString(), QString(), true);
    return IRBuilder().generateFromQml(source, QString(), documentPtr.get())
            ? std::move(documentPtr)
            : nullptr;
}

/*
This test ensures that virtual specifiers are correctly propagated from the QML source code into the
intermediate representation (IR).

Due to the current design, it is difficult (if not impossible) to test
IRBuilder::visit(UiPublicMember*) in complete isolation.

Therefore, this test takes an end-to-end (E2E) approach: it constructs a valid QML object
declaration containing a property with the relevant specifier keywords, builds the IR for it, and
then verifies that the resulting property node in the IR has the expected flags.*/
void tst_qmljsir::propertyVirtSpecifiers()
{
    QFETCH(Phrase, virtSpecifiers);
    QFETCH(Property, propertyWithVirtFlags);

    Phrase propertyDeclaration =
            virtSpecifiers + Phrase{ Token::T_PROPERTY, Token::T_VAR, QLatin1StringView("p") };
    const auto source = toString(objectDeclaration(std::move(propertyDeclaration)));

    const auto documentPtr = tryBuildIRDocumentFrom(source);
    QVERIFY(documentPtr);
    QCOMPARE(documentPtr->objectCount(), 1);
    QCOMPARE(documentPtr->objectAt(0)->propertyCount(), 1);

    const auto property = documentPtr->objectAt(0)->firstProperty();
    QVERIFY(property);
    QCOMPARE(property->isVirtual(), propertyWithVirtFlags.isVirtual());
    QCOMPARE(property->isOverride(), propertyWithVirtFlags.isOverride());
    QCOMPARE(property->isFinal(), propertyWithVirtFlags.isFinal());
}

QTEST_MAIN(tst_qmljsir)
