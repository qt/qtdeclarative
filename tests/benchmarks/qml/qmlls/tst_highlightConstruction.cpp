// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQmlLS/private/qqmlsemantictokens_p.h>
#include <QtQmlDom/private/qqmldomitem_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>

#include <QtTest/QTest>
#include <QFile>
#include <QLibraryInfo>

static std::pair<QQmlJS::Dom::DomItem, QString> fileObject(QLatin1StringView path)
{
    using namespace QQmlJS::Dom;
    DomItem file;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    QString code(f.readAll());
    QStringList dirs = { QLibraryInfo::path(QLibraryInfo::Qml2ImportsPath) };
    auto envPtr = DomEnvironment::create(dirs, QQmlJS::Dom::DomEnvironment::Option::SingleThreaded,
                                         Extended);
    envPtr->loadBuiltins();
    envPtr->loadFile(
            FileToLoad::fromMemory(envPtr, path, code),
            [&file](Path, const DomItem &, const DomItem &newIt) { file = newIt.fileObject(); });
    envPtr->loadPendingDependencies();
    return { file, code };
};

static constexpr QLatin1StringView benchmarkFiles[] = {
       QLatin1StringView(EXAMPLESDIR "/quick/quickshapes/shapes/tiger.qml"), // big file
       QLatin1StringView(EXAMPLESDIR "/quick/quickshapes/shapes/zoomtiger.qml") // small file
};

class tst_highlightConstruction : public QObject
{
    Q_OBJECT
private slots:
    void highlightConstruction_data();
    void highlightConstruction();

    void highlightShifting_data();
    void highlightShifting();

    void regexFallbackHighlightConstruction_data();
    void regexFallbackHighlightConstruction();
};

void tst_highlightConstruction::highlightConstruction_data()
{
    QTest::addColumn<QLatin1StringView>("filePath");

    QTest::addRow("bigFile") << benchmarkFiles[0]; // big file
    QTest::addRow("smallFile") << benchmarkFiles[1]; // small file
}

void tst_highlightConstruction::highlightConstruction()
{
    using namespace QQmlJS::Dom;
    QFETCH(QLatin1StringView, filePath);
    const auto [fileItem, code] = fileObject(filePath);
    QVERIFY(fileItem.field(Fields::isValid).value().toBool());
    QBENCHMARK {
        QmlHighlighting::HighlightsContainer originalHighlights =
                QmlHighlighting::Utils::visitTokens(fileItem, std::nullopt);
        QVERIFY(!originalHighlights.isEmpty());
    }
}

void tst_highlightConstruction::highlightShifting_data()
{
    QTest::addColumn<QLatin1StringView>("filePath");
    QTest::addColumn<qsizetype>("startOffsetModification"); // to simulate small edits
    QTest::addColumn<qsizetype>("lengthOfModification"); // to simulate small edits

    QTest::addRow("bigFile") << benchmarkFiles[0] << 156ll << 3ll; // big file
    QTest::addRow("smallFile") << benchmarkFiles[1] << 184ll << 3ll; // small file
}

void tst_highlightConstruction::highlightShifting()
{
    using namespace QQmlJS::Dom;
    QFETCH(QLatin1StringView, filePath);
    QFETCH(qsizetype, startOffsetModification);
    QFETCH(qsizetype, lengthOfModification);
    const auto [fileItem, code] = fileObject(filePath);
    QmlHighlighting::HighlightsContainer originalHighlights =
            QmlHighlighting::Utils::visitTokens(fileItem, std::nullopt);
    QVERIFY(!originalHighlights.isEmpty());

    QString modifiedCode = code;
    modifiedCode.remove(startOffsetModification, lengthOfModification); // simulate small edit
    QBENCHMARK {
        QmlHighlighting::HighlightsContainer shiftedHighlights =
                QmlHighlighting::Utils::shiftHighlights(originalHighlights, code, modifiedCode);
        QVERIFY(!shiftedHighlights.isEmpty());
    }
}

// regexFallbackHighlights() is the line-based regex scanner used when a document cannot be
// parsed into a DomItem at all (e.g. while the user is mid-edit). It is deliberately much
// simpler than the DOM-based visitTokens() used by highlightConstruction() above, so this
// benchmark exists to quantify that difference on the same input files.
void tst_highlightConstruction::regexFallbackHighlightConstruction_data()
{
    QTest::addColumn<QLatin1StringView>("filePath");

    QTest::addRow("bigFile") << benchmarkFiles[0]; // big file
    QTest::addRow("smallFile") << benchmarkFiles[1]; // small file
}

void tst_highlightConstruction::regexFallbackHighlightConstruction()
{
    using namespace QQmlJS::Dom;
    QFETCH(QLatin1StringView, filePath);
    const auto [fileItem, code] = fileObject(filePath);
    QVERIFY(fileItem.field(Fields::isValid).value().toBool());
    QBENCHMARK {
        QmlHighlighting::HighlightsContainer fallbackHighlights =
                QmlHighlighting::Utils::regexFallbackHighlights(code, std::nullopt);
        QVERIFY(!fallbackHighlights.isEmpty());
    }
}

QTEST_MAIN(tst_highlightConstruction)
#include "tst_highlightConstruction.moc"
