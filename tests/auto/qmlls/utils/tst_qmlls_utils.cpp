// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tst_qmlls_utils.h"

// some helper constants for the tests
const static int positionAfterOneIndent = 5;
// constants for resultIndex
const static int firstResult = 0;
const static int secondResult = 1;
// constants for expectedItemsCount
const static int outOfOne = 1;
const static int outOfTwo = 2;

std::tuple<QQmlJS::Dom::DomItem, QQmlJS::Dom::DomItem>
tst_qmlls_utils::createEnvironmentAndLoadFile(const QString &filePath)
{
    if (auto entry = cache.find(filePath); entry != cache.end())
        return *entry;

    QStringList qmltypeDirs =
            QStringList({ dataDirectory(), QLibraryInfo::path(QLibraryInfo::Qml2ImportsPath) });

    QQmlJS::Dom::DomItem env = QQmlJS::Dom::DomEnvironment::create(
            qmltypeDirs,
            QQmlJS::Dom::DomEnvironment::Option::
                    SingleThreaded); // | QQmlJS::Dom::DomEnvironment::Option::NoDependencies);

    QQmlJS::Dom::DomItem file;
    env.loadFile(
            filePath, QString(),
            [&file](QQmlJS::Dom::Path, const QQmlJS::Dom::DomItem &,
                    const QQmlJS::Dom::DomItem &newIt) { file = newIt; },
            QQmlJS::Dom::LoadOption::DefaultLoad);

    env.loadPendingDependencies();
    env.loadBuiltins();

    return cache[filePath] = std::make_tuple(env, file);
}

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
    // keep in mind that line and character are starting at 1!
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("character");
    // in case there are multiple items to be found (e.g. for a location between two objects), the
    // item to be checked against
    QTest::addColumn<int>("resultIndex");
    QTest::addColumn<int>("expectedItemsCount");
    QTest::addColumn<QQmlJS::Dom::DomType>("expectedType");
    // set to -1 when unchanged from above line and character, starts at 1
    QTest::addColumn<int>("expectedLine");
    QTest::addColumn<int>("expectedCharacter");

    const QString file1Qml = testFile(u"file1.qml"_s);

    QTest::addRow("findIntProperty") << file1Qml << 9 << 18 << firstResult << outOfOne
                                     << QQmlJS::Dom::DomType::PropertyDefinition
                                     // start of the "property"-token of the "a" property
                                     << -1 << positionAfterOneIndent;
    QTest::addRow("findIntProperty2") << file1Qml << 9 << 10 << firstResult << outOfOne
                                      << QQmlJS::Dom::DomType::PropertyDefinition
                                      // start of the "property"-token of the "a" property
                                      << -1 << positionAfterOneIndent;
    QTest::addRow("findIntBinding")
            << file1Qml << 30 << positionAfterOneIndent << firstResult << outOfOne
            << QQmlJS::Dom::DomType::Binding
            // start of the a identifier of the "a" binding
            << -1 << positionAfterOneIndent;
    QTest::addRow("findIntBinding2") << file1Qml << 30 << 8 << firstResult << outOfOne
                                     << QQmlJS::Dom::DomType::Binding
                                     // start of the a identifier of the "a" binding
                                     << -1 << positionAfterOneIndent;

    QTest::addRow("colorBinding") << file1Qml << 39 << 13 << firstResult << outOfOne
                                  << QQmlJS::Dom::DomType::Binding << -1
                                  << 2 * positionAfterOneIndent - 1;

    QTest::addRow("findVarProperty") << file1Qml << 12 << 12 << firstResult << outOfOne
                                     << QQmlJS::Dom::DomType::PropertyDefinition
                                     // start of the "property"-token of the "d" property
                                     << -1 << positionAfterOneIndent;
    QTest::addRow("findVarBinding") << file1Qml << 31 << 8 << firstResult << outOfOne
                                    << QQmlJS::Dom::DomType::Binding
                                    // start of the "property"-token of the "d" property
                                    << -1 << positionAfterOneIndent;
    QTest::addRow("beforeEProperty")
            << file1Qml << 13 << positionAfterOneIndent << firstResult << outOfOne
            << QQmlJS::Dom::DomType::PropertyDefinition
            // start of the "property"-token of the "e" property
            << -1 << -1;
    QTest::addRow("onEProperty") << file1Qml << 13 << 24 << firstResult << outOfOne
                                 << QQmlJS::Dom::DomType::PropertyDefinition
                                 // start of the "property"-token of the "e" property
                                 << -1 << positionAfterOneIndent;
    QTest::addRow("afterEProperty") << file1Qml << 13 << 25 << firstResult << outOfOne
                                    << QQmlJS::Dom::DomType::PropertyDefinition
                                    // start of the "property"-token of the "e" property
                                    << -1 << positionAfterOneIndent;

    QTest::addRow("property-in-ic") << file1Qml << 28 << 36 << firstResult << outOfOne
                                    << QQmlJS::Dom::DomType::PropertyDefinition << -1 << 26;

    QTest::addRow("onCChild") << file1Qml << 16 << positionAfterOneIndent << firstResult << outOfOne
                              << QQmlJS::Dom::DomType::QmlObject << -1 << positionAfterOneIndent;

    // check for off-by-one/overlapping items
    QTest::addRow("closingBraceOfC")
            << file1Qml << 16 << 19 << firstResult << outOfOne << QQmlJS::Dom::DomType::QmlObject
            << -1 << positionAfterOneIndent;
    QTest::addRow("beforeClosingBraceOfC") << file1Qml << 16 << 18 << firstResult << outOfOne
                                           << QQmlJS::Dom::DomType::Id << -1 << 8;
    QTest::addRow("firstBetweenCandD")
            << file1Qml << 16 << 20 << secondResult << outOfTwo << QQmlJS::Dom::DomType::QmlObject
            << -1 << positionAfterOneIndent;
    QTest::addRow("secondBetweenCandD") << file1Qml << 16 << 20 << firstResult << outOfTwo
                                        << QQmlJS::Dom::DomType::QmlObject << -1 << -1;

    QTest::addRow("afterD") << file1Qml << 16 << 21 << firstResult << outOfOne
                            << QQmlJS::Dom::DomType::QmlObject << -1 << 20;

    // check what happens between items (it should not crash)

    QTest::addRow("onWhitespaceBeforeC")
            << file1Qml << 16 << 1 << firstResult << outOfOne << QQmlJS::Dom::DomType::Map << 9
            << positionAfterOneIndent;

    QTest::addRow("onWhitespaceAfterC")
            << file1Qml << 17 << 8 << firstResult << outOfOne << QQmlJS::Dom::DomType::QmlObject
            << -1 << positionAfterOneIndent;

    QTest::addRow("onWhitespaceBetweenCAndD") << file1Qml << 17 << 23 << firstResult << outOfOne
                                              << QQmlJS::Dom::DomType::Map << 16 << 8;
    QTest::addRow("onWhitespaceBetweenCAndD2") << file1Qml << 17 << 24 << firstResult << outOfOne
                                               << QQmlJS::Dom::DomType::Map << 16 << 8;

    // check workaround for inline components
    QTest::addRow("ic") << file1Qml << 15 << 15 << firstResult << outOfOne
                        << QQmlJS::Dom::DomType::QmlComponent << -1 << 5;
    QTest::addRow("ic2") << file1Qml << 15 << 20 << firstResult << outOfOne
                         << QQmlJS::Dom::DomType::QmlObject << -1 << 18;
    QTest::addRow("ic3") << file1Qml << 15 << 33 << firstResult << outOfOne
                         << QQmlJS::Dom::DomType::Id << -1 << 25;

    QTest::addRow("function") << file1Qml << 33 << 5 << firstResult << outOfOne
                              << QQmlJS::Dom::DomType::MethodInfo << -1 << positionAfterOneIndent;
    QTest::addRow("function-parameter") << file1Qml << 33 << 20 << firstResult << outOfOne
                                        << QQmlJS::Dom::DomType::MethodParameter << -1 << 16;
    // The return type of a function has no own DomItem. Instead, the return type of a function
    // is saved into the MethodInfo.
    QTest::addRow("function-return")
            << file1Qml << 33 << 41 << firstResult << outOfOne << QQmlJS::Dom::DomType::MethodInfo
            << -1 << positionAfterOneIndent;
    QTest::addRow("function2") << file1Qml << 36 << 17 << firstResult << outOfOne
                               << QQmlJS::Dom::DomType::MethodInfo << -1 << positionAfterOneIndent;

    // check rectangle property
    QTest::addRow("rectangle-property") << file1Qml << 44 << 31 << firstResult << outOfOne
                                        << QQmlJS::Dom::DomType::Binding << -1 << 24;
}

void tst_qmlls_utils::findItemFromLocation()
{
    QFETCH(QString, filePath);
    QFETCH(int, line);
    QFETCH(int, character);
    QFETCH(int, resultIndex);
    QFETCH(int, expectedItemsCount);
    QFETCH(QQmlJS::Dom::DomType, expectedType);
    QFETCH(int, expectedLine);
    QFETCH(int, expectedCharacter);

    if (expectedLine == -1)
        expectedLine = line;
    if (expectedCharacter == -1)
        expectedCharacter = character;

    // they all start at 1.
    Q_ASSERT(line > 0);
    Q_ASSERT(character > 0);
    Q_ASSERT(expectedLine > 0);
    Q_ASSERT(expectedCharacter > 0);

    auto [env, file] = createEnvironmentAndLoadFile(filePath);

    auto locations = QQmlLSUtils::itemsFromTextLocation(
            file.field(QQmlJS::Dom::Fields::currentItem), line - 1, character - 1);

    QVERIFY(resultIndex < locations.size());
    QCOMPARE(locations.size(), expectedItemsCount);

    QQmlJS::Dom::DomItem itemToTest = locations[resultIndex].domItem;
    // ask for the type in the args
    // if (itemToTest.internalKind() != expectedType)
    //     qDebug() << itemToTest.internalKindStr() << " has not the expected kind " << expectedType
    //              << " for item " << itemToTest.toString();
    QCOMPARE(itemToTest.internalKind(), expectedType);

    QQmlJS::Dom::FileLocations::Tree locationToTest = locations[resultIndex].fileLocation;
    QCOMPARE(locationToTest->info().fullRegion.startLine, quint32(expectedLine));
    QCOMPARE(locationToTest->info().fullRegion.startColumn, quint32(expectedCharacter));
}

void tst_qmlls_utils::findLocationOfItem_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("character");
    QTest::addColumn<int>("expectedLine");
    QTest::addColumn<int>("expectedCharacter");

    const QString file1Qml = testFile(u"file1.qml"_s);

    QTest::addRow("root-element") << file1Qml << 6 << 2 << -1 << 1;

    QTest::addRow("property-a") << file1Qml << 9 << 17 << -1 << positionAfterOneIndent;
    QTest::addRow("property-a2") << file1Qml << 9 << 10 << -1 << positionAfterOneIndent;
    QTest::addRow("nested-C") << file1Qml << 20 << 9 << -1 << -1;
    QTest::addRow("nested-C2") << file1Qml << 23 << 13 << -1 << -1;
    QTest::addRow("D") << file1Qml << 17 << 33 << -1 << 28;
    QTest::addRow("property-d") << file1Qml << 12 << 15 << -1 << positionAfterOneIndent;

    QTest::addRow("import") << file1Qml << 4 << 6 << -1 << 1;
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

    // they all start at 1.
    Q_ASSERT(line > 0);
    Q_ASSERT(character > 0);
    Q_ASSERT(expectedLine > 0);
    Q_ASSERT(expectedCharacter > 0);

    auto [env, file] = createEnvironmentAndLoadFile(filePath);

    // grab item using already tested QQmlLSUtils::findLastItemsContaining
    auto locations = QQmlLSUtils::itemsFromTextLocation(
            file.field(QQmlJS::Dom::Fields::currentItem), line - 1, character - 1);
    QCOMPARE(locations.size(), 1);

    // once the item is grabbed, make sure its line/character position can be obtained back
    auto t = QQmlLSUtils::textLocationFromItem(locations.front().domItem);

    QCOMPARE(t->info().fullRegion.startLine, quint32(expectedLine));
    QCOMPARE(t->info().fullRegion.startColumn, quint32(expectedCharacter));
}

QTEST_MAIN(tst_qmlls_utils)
