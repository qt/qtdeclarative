// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tst_qmlls_utils.h"

// some helper constants for the tests
const static int indentWidth = 4;
// constants for resultIndex
const static int firstResult = 0;
const static int secondResult = 1;
// constants for expectedItemsCount
const static int outOfOne = 1;
const static int outOfTwo = 2;

void tst_qmlls_utils::textOffsetRowColumnConversions_data()
{
    QTest::addColumn<QString>("code");
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("character");
    QTest::addColumn<qsizetype>("expectedOffset");
    QTest::addColumn<QChar>("expectedChar");
    // in case they differ from line and character, e.g. when accessing non-existing line or rows
    // set to -1 when same as before
    QTest::addColumn<int>("expectedLine");
    QTest::addColumn<int>("expectedCharacter");

    QTest::newRow("oneline") << u"Hello World!"_s << 0 << 6 << 6ll << QChar('W') << -1 << -1;
    QTest::newRow("multi-line") << u"Hello World!\n How are you? \n Bye!\n"_s << 0 << 6 << 6ll
                                << QChar('W') << -1 << -1;
    QTest::newRow("multi-line2") << u"Hello World!\n How are you? \n Bye!\n"_s << 1 << 5 << 18ll
                                 << QChar('a') << -1 << -1;
    QTest::newRow("multi-line3") << u"Hello World!\n How are you? \n Bye!\n"_s << 2 << 1 << 29ll
                                 << QChar('B') << -1 << -1;

    QTest::newRow("newlines") << u"A\nB\r\nC\n\r\nD\r\n\r"_s << 0 << 0 << 0ll << QChar('A') << -1
                              << -1;
    QTest::newRow("newlines2") << u"A\nB\r\nC\n\r\nD\r\n\r"_s << 1 << 0 << 2ll << QChar('B') << -1
                               << -1;

    // try to access '\r'
    QTest::newRow("newlines3") << u"A\nB\r\nC\n\r\nD\r\n\r"_s << 1 << 1 << 3ll << QChar('\r') << -1
                               << -1;
    // try to access '\n', should return the last character of the line (which is '\r' in this case)
    QTest::newRow("newlines4") << u"A\nB\r\nC\n\r\nD\r\n\r"_s << 1 << 2 << 3ll << QChar('\r') << -1
                               << 1;
    // try to access after the end of the line, should return the last character of the line (which
    // is '\r' in this case)
    QTest::newRow("afterLineEnd") << u"A\nB\r\nC\n\r\nD\r\n\r"_s << 1 << 42 << 3ll << QChar('\r')
                                  << -1 << 1;

    // try to access an inexisting column, seems to return the last character of the last line.
    QTest::newRow("afterColumnEnd")
            << u"A\nB\r\nC\n\r\nD\r\n\rAX"_s << 42 << 0 << 15ll << QChar('X') << 5 << 2;
}

void tst_qmlls_utils::textOffsetRowColumnConversions()
{
    QFETCH(QString, code);
    QFETCH(int, line);
    QFETCH(int, character);
    QFETCH(qsizetype, expectedOffset);
    QFETCH(QChar, expectedChar);
    QFETCH(int, expectedLine);
    QFETCH(int, expectedCharacter);

    qsizetype offset = QQmlLSUtils::textOffsetFrom(code, line, character);

    QCOMPARE(offset, expectedOffset);
    if (offset < code.size())
        QCOMPARE(code[offset], expectedChar);

    auto [computedRow, computedColumn] = QQmlLSUtils::textRowAndColumnFrom(code, expectedOffset);
    if (expectedLine == -1)
        expectedLine = line;
    if (expectedCharacter == -1)
        expectedCharacter = character;

    QCOMPARE(computedRow, expectedLine);
    QCOMPARE(computedColumn, expectedCharacter);
}

void tst_qmlls_utils::findItemFromLocation_data()
{
    QTest::addColumn<QString>("filePath");
    // keep in mind that line and character are 0-based!
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("character");
    // in case there are multiple items to be found (e.g. for a location between two objects), the
    // item to be checked against
    QTest::addColumn<int>("resultIndex");
    QTest::addColumn<int>("expectedItemsCount");
    QTest::addColumn<QQmlJS::Dom::DomType>("expectedType");
    QTest::addColumn<QString>("expectedName");
    // set to -1 when unchanged from above line and character. 0-based.
    QTest::addColumn<int>("expectedLine");
    QTest::addColumn<int>("expectedCharacter");

    QTest::addRow("findIntProperty") << testFile(u"file1.qml"_s) << 5 << 17 << firstResult
                                     << outOfOne << QQmlJS::Dom::DomType::PropertyDefinition
                                     << u"a"_s
                                     // start of the "property"-token of the "a" property
                                     << -1 << indentWidth;
    QTest::addRow("findIntProperty2") << testFile(u"file1.qml"_s) << 5 << 9 << firstResult
                                      << outOfOne << QQmlJS::Dom::DomType::PropertyDefinition
                                      << u"a"_s
                                      // start of the "property"-token of the "a" property
                                      << -1 << indentWidth;
    QTest::addRow("findVarProperty") << testFile(u"file1.qml"_s) << 8 << 11 << firstResult
                                     << outOfOne << QQmlJS::Dom::DomType::PropertyDefinition
                                     << u"d"_s
                                     // start of the "property"-token of the "d" property
                                     << -1 << indentWidth;
    QTest::addRow("beforeEProperty") << testFile(u"file1.qml"_s) << 9 << indentWidth << firstResult
                                     << outOfOne << QQmlJS::Dom::DomType::PropertyDefinition
                                     << u"e"_s
                                     // start of the "property"-token of the "e" property
                                     << -1 << -1;
    QTest::addRow("onEProperty") << testFile(u"file1.qml"_s) << 9 << 23 << firstResult << outOfOne
                                 << QQmlJS::Dom::DomType::PropertyDefinition
                                 << u"e"_s
                                 // start of the "property"-token of the "e" property
                                 << -1 << indentWidth;
    QTest::addRow("afterEProperty") << testFile(u"file1.qml"_s) << 9 << 24 << firstResult
                                    << outOfOne << QQmlJS::Dom::DomType::PropertyDefinition
                                    << u"e"_s
                                    // start of the "property"-token of the "e" property
                                    << -1 << indentWidth;

    QTest::addRow("property-in-ic")
            << testFile(u"file1.qml"_s) << 24 << 35 << firstResult << outOfOne
            << QQmlJS::Dom::DomType::PropertyDefinition << u"myC"_s << 24 << 25;

    QTest::addRow("onCChild") << testFile(u"file1.qml"_s) << 12 << 4 << firstResult << outOfOne
                              << QQmlJS::Dom::DomType::QmlObject << u"C"_s << -1 << indentWidth;

    // check for off-by-one/overlapping items
    QTest::addRow("closingBraceOfC")
            << testFile(u"file1.qml"_s) << 12 << 17 << firstResult << outOfOne
            << QQmlJS::Dom::DomType::QmlObject << u"C"_s << -1 << indentWidth;
    QTest::addRow("firstBetweenCandD")
            << testFile(u"file1.qml"_s) << 12 << 18 << secondResult << outOfTwo
            << QQmlJS::Dom::DomType::QmlObject << u"C"_s << -1 << indentWidth;
    QTest::addRow("secondBetweenCandD")
            << testFile(u"file1.qml"_s) << 12 << 18 << firstResult << outOfTwo
            << QQmlJS::Dom::DomType::QmlObject << u"D"_s << -1 << -1;

    QTest::addRow("afterD") << testFile(u"file1.qml"_s) << 12 << 19 << firstResult << outOfOne
                            << QQmlJS::Dom::DomType::QmlObject << u"D"_s << -1 << 18;

    // check what happens between items:

    QTest::addRow("onWhitespaceBeforeC")
            << testFile(u"file1.qml"_s) << 12 << 0 << firstResult << outOfOne
            << QQmlJS::Dom::DomType::QmlObject << u"C"_s << -1 << indentWidth;

    QTest::addRow("onWhitespaceAfterC")
            << testFile(u"file1.qml"_s) << 13 << 8 << firstResult << outOfOne
            << QQmlJS::Dom::DomType::QmlObject << u"C"_s << -1 << indentWidth;

    QTest::addRow("onWhitespaceBetweenCAndD")
            << testFile(u"file1.qml"_s) << 13 << 23 << firstResult << outOfOne
            << QQmlJS::Dom::DomType::QmlObject << u"D"_s << -1 << 24;
    QTest::addRow("onWhitespaceBetweenCAndD2")
            << testFile(u"file1.qml"_s) << 13 << 22 << firstResult << outOfOne
            << QQmlJS::Dom::DomType::QmlObject << u"D"_s << -1 << 24;
    // check workaround for inline components
    QTest::addRow("ic") << testFile(u"file1.qml"_s) << 11 << 14 << firstResult << outOfOne
                        << QQmlJS::Dom::DomType::QmlObject << u"Item"_s << -1 << 17;
    QTest::addRow("ic2") << testFile(u"file1.qml"_s) << 11 << 19 << firstResult << outOfOne
                         << QQmlJS::Dom::DomType::QmlObject << u"Item"_s << -1 << 17;
    QTest::addRow("ic3") << testFile(u"file1.qml"_s) << 11 << 32 << firstResult << outOfOne
                         << QQmlJS::Dom::DomType::Id << u"icid"_s << -1 << 24;

}

std::tuple<QQmlJS::Dom::DomItem, QQmlJS::Dom::DomItem>
tst_qmlls_utils::createEnvironmentAndLoadFile(const QString &filePath) const
{
    QStringList qmltypeDirs =
            QStringList({ dataDirectory(), QLibraryInfo::path(QLibraryInfo::Qml2ImportsPath) });

    QQmlJS::Dom::DomItem env = QQmlJS::Dom::DomEnvironment::create(
            qmltypeDirs,
            QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                    | QQmlJS::Dom::DomEnvironment::Option::NoDependencies);

    QQmlJS::Dom::DomItem file;
    env.loadFile(
            filePath, QString(),
            [&file](QQmlJS::Dom::Path, const QQmlJS::Dom::DomItem &,
                    const QQmlJS::Dom::DomItem &newIt) { file = newIt; },
            QQmlJS::Dom::LoadOption::DefaultLoad);

    env.loadPendingDependencies();

    return { env, file };
}

void tst_qmlls_utils::findItemFromLocation()
{
    QFETCH(QString, filePath);
    QFETCH(int, line);
    QFETCH(int, character);
    QFETCH(int, resultIndex);
    QFETCH(int, expectedItemsCount);
    QFETCH(QQmlJS::Dom::DomType, expectedType);
    QFETCH(QString, expectedName);
    QFETCH(int, expectedLine);
    QFETCH(int, expectedCharacter);

    if (expectedLine == -1)
        expectedLine = line;
    if (expectedCharacter == -1)
        expectedCharacter = character;

    auto [env, file] = createEnvironmentAndLoadFile(filePath);

    auto locations = QQmlLSUtils::itemsFromTextLocation(
            file.field(QQmlJS::Dom::Fields::currentItem), line, character,
            QQmlLSUtils::IgnoreWhitespace::OnSameLine);

    QVERIFY(resultIndex < locations.size());
    QCOMPARE(locations.size(), expectedItemsCount);

    QQmlJS::Dom::DomItem itemToTest = locations[resultIndex].domItem;
    // ask for the type in the args
    QCOMPARE(itemToTest.internalKind(), expectedType);
    QCOMPARE(itemToTest.name(), expectedName);

    QQmlJS::Dom::FileLocations::Tree locationToTest = locations[resultIndex].fileLocation;
    QCOMPARE(locationToTest->info().fullRegion.startLine, quint32(expectedLine + 1));
    QCOMPARE(locationToTest->info().fullRegion.startColumn, quint32(expectedCharacter + 1));
}

void tst_qmlls_utils::findLocationOfItem_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("character");
    QTest::addColumn<int>("expectedLine");
    QTest::addColumn<int>("expectedCharacter");

    QTest::addRow("root-element") << testFile(u"file1.qml"_s) << 2 << 2 << -1 << 0;

    QTest::addRow("property-a") << testFile(u"file1.qml"_s) << 5 << 17 << -1 << indentWidth;
    QTest::addRow("property-a2") << testFile(u"file1.qml"_s) << 5 << 9 << -1 << indentWidth;
    QTest::addRow("nested-C") << testFile(u"file1.qml"_s) << 16 << 8 << -1 << -1;
    QTest::addRow("nested-C2") << testFile(u"file1.qml"_s) << 19 << 12 << -1 << -1;
    QTest::addRow("D") << testFile(u"file1.qml"_s) << 13 << 32 << -1 << 24;
    QTest::addRow("property-d") << testFile(u"file1.qml"_s) << 8 << 14 << -1 << indentWidth;

    QTest::addRow("import") << testFile(u"file1.qml"_s) << 0 << 5 << -1 << 0;
}

void tst_qmlls_utils::findLocationOfItem()
{
    QFETCH(QString, filePath);
    QFETCH(int, line);
    QFETCH(int, character);
    QFETCH(int, expectedLine);
    QFETCH(int, expectedCharacter);

    if (expectedLine == -1)
        expectedLine = line;
    if (expectedCharacter == -1)
        expectedCharacter = character;

    auto [env, file] = createEnvironmentAndLoadFile(filePath);

    // grab item using already tested QQmlLSUtils::findLastItemsContaining
    auto locations = QQmlLSUtils::itemsFromTextLocation(
            file.field(QQmlJS::Dom::Fields::currentItem), line, character,
            QQmlLSUtils::IgnoreWhitespace::OnSameLine);
    QCOMPARE(locations.size(), 1);

    // once the item is grabbed, make sure its line/character position can be obtained back
    auto t = QQmlLSUtils::textLocationFromItem(locations.front().domItem);

    QCOMPARE(t->info().fullRegion.startLine, quint32(expectedLine + 1));
    QCOMPARE(t->info().fullRegion.startColumn, quint32(expectedCharacter + 1));
}

QTEST_MAIN(tst_qmlls_utils)
