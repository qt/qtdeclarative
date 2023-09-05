// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tst_qmlls_utils.h"
#include <algorithm>
#include <optional>

// some helper constants for the tests
const static int positionAfterOneIndent = 5;
const static QString noResultExpected;
// constants for resultIndex
const static int firstResult = 0;
const static int secondResult = 1;
// constants for expectedItemsCount
const static int outOfOne = 1;
const static int outOfTwo = 2;

// enable/disable additional debug output
constexpr static bool enable_debug_output = true;

static QString printSet(const QSet<QString> &s)
{
    const QString r = QStringList(s.begin(), s.end()).join(u", "_s);
    return r;
}

std::tuple<QQmlJS::Dom::DomItem, QQmlJS::Dom::DomItem>
tst_qmlls_utils::createEnvironmentAndLoadFile(const QString &filePath,
                                              QQmlJS::Dom::DomCreationOptions options)
{
    CacheKey cacheKey = { filePath, options };
    if (auto entry = cache.find(cacheKey); entry != cache.end())
        return *entry;

    QStringList qmltypeDirs =
            QStringList({ dataDirectory(), QLibraryInfo::path(QLibraryInfo::Qml2ImportsPath) });

    QQmlJS::Dom::DomItem env = QQmlJS::Dom::DomEnvironment::create(
            qmltypeDirs, QQmlJS::Dom::DomEnvironment::Option::SingleThreaded);

    QQmlJS::Dom::DomItem file;
    env.loadFile(
            QQmlJS::Dom::FileToLoad::fromFileSystem(env.ownerAs<QQmlJS::Dom::DomEnvironment>(),
                                                    filePath, options),
            [&file](QQmlJS::Dom::Path, const QQmlJS::Dom::DomItem &,
                    const QQmlJS::Dom::DomItem &newIt) { file = newIt; },
            QQmlJS::Dom::LoadOption::DefaultLoad);

    env.loadPendingDependencies();
    env.loadBuiltins();

    return cache[cacheKey] = std::make_tuple(env, file);
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

    QQmlJS::Dom::DomCreationOptions options;
    options.setFlag(QQmlJS::Dom::DomCreationOption::WithSemanticAnalysis);

    auto [env, file] = createEnvironmentAndLoadFile(filePath, options);

    auto locations = QQmlLSUtils::itemsFromTextLocation(
            file.field(QQmlJS::Dom::Fields::currentItem), line - 1, character - 1);

    QVERIFY(resultIndex < locations.size());
    QCOMPARE(locations.size(), expectedItemsCount);

    QQmlJS::Dom::DomItem itemToTest = locations[resultIndex].domItem;
    // ask for the type in the args
    if constexpr (enable_debug_output) {
        if (itemToTest.internalKind() != expectedType) {
            qDebug() << itemToTest.internalKindStr() << " has not the expected kind "
                     << expectedType << " for item " << itemToTest.toString();
        }
    }
    QCOMPARE(itemToTest.internalKind(), expectedType);

    QQmlJS::Dom::FileLocations::Tree locationToTest = locations[resultIndex].fileLocation;
    QCOMPARE(locationToTest->info().fullRegion.startLine, quint32(expectedLine));
    QCOMPARE(locationToTest->info().fullRegion.startColumn, quint32(expectedCharacter));
}

void tst_qmlls_utils::findTypeDefinitionFromLocation_data()
{
    QTest::addColumn<QString>("filePath");
    // keep in mind that line and character are starting at 1!
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("character");
    // in case there are multiple items to be found (e.g. for a location between two objects), the
    // item to be checked against
    QTest::addColumn<int>("resultIndex");
    QTest::addColumn<int>("expectedItemsCount");
    QTest::addColumn<QString>("expectedFilePath");
    // set to -1 when unchanged from above line and character. 0-based.
    QTest::addColumn<int>("expectedLine");
    QTest::addColumn<int>("expectedCharacter");

    const QString file1Qml = testFile(u"file1.qml"_s);
    const QString TypeQml = testFile(u"Type.qml"_s);
    // pass this as file when no result is expected, e.g. for type definition of "var".

    QTest::addRow("onCProperty") << file1Qml << 11 << 16 << firstResult << outOfOne << file1Qml << 7
                                 << positionAfterOneIndent;

    QTest::addRow("onCProperty2") << file1Qml << 28 << 37 << firstResult << outOfOne << file1Qml
                                  << 7 << positionAfterOneIndent;

    QTest::addRow("onCProperty3") << file1Qml << 28 << 35 << firstResult << outOfOne << file1Qml
                                  << 7 << positionAfterOneIndent;

    QTest::addRow("onCBinding") << file1Qml << 46 << 8 << firstResult << outOfOne << file1Qml << 7
                                << positionAfterOneIndent;

    QTest::addRow("onDefaultBinding") << file1Qml << 16 << positionAfterOneIndent << firstResult
                                      << outOfOne << file1Qml << 7 << positionAfterOneIndent;

    QTest::addRow("onDefaultBindingId")
            << file1Qml << 16 << 28 << firstResult << outOfOne << file1Qml << 16 << 20;

    QTest::addRow("findIntProperty") << file1Qml << 9 << 18 << firstResult << outOfOne << file1Qml
                                     << -1 << positionAfterOneIndent;
    QTest::addRow("colorBinding") << file1Qml << 39 << 8 << firstResult << outOfOne << file1Qml
                                  << -1 << positionAfterOneIndent;

    // check what happens between items (it should not crash)

    QTest::addRow("onWhitespaceBeforeC")
            << file1Qml << 16 << 1 << firstResult << outOfOne << noResultExpected << -1 << -1;

    QTest::addRow("onWhitespaceAfterC")
            << file1Qml << 17 << 23 << firstResult << outOfOne << noResultExpected << -1 << -1;

    QTest::addRow("onWhitespaceBetweenCAndD")
            << file1Qml << 17 << 24 << firstResult << outOfOne << noResultExpected << -1 << -1;

    QTest::addRow("ic") << file1Qml << 15 << 15 << firstResult << outOfOne << file1Qml << -1 << 18;
    QTest::addRow("icBase") << file1Qml << 15 << 20 << firstResult << outOfOne
                            << u"TODO: file location for C++ defined types?"_s << -1 << -1;
    QTest::addRow("ic3") << file1Qml << 15 << 33 << firstResult << outOfOne << file1Qml << -1 << 18;

    // TODO: type definition of function = type definition of return type?
    // if not, this might need fixing:
    // currently, asking the type definition of the "function" keyword returns
    // the type definitin of the return type (when available).
    QTest::addRow("function-keyword") << file1Qml << 33 << 5 << firstResult << outOfOne << file1Qml
                                      << 7 << positionAfterOneIndent;
    QTest::addRow("function-parameter-builtin")
            << file1Qml << 33 << 20 << firstResult << outOfOne << file1Qml << -1 << -1;
    QTest::addRow("function-parameter-item") << file1Qml << 33 << 36 << firstResult << outOfOne
                                             << file1Qml << 7 << positionAfterOneIndent;

    QTest::addRow("function-return") << file1Qml << 33 << 41 << firstResult << outOfOne << file1Qml
                                     << 7 << positionAfterOneIndent;

    QTest::addRow("void-function")
            << file1Qml << 36 << 17 << firstResult << outOfOne << noResultExpected << -1 << -1;

    QTest::addRow("rectangle-property") << file1Qml << 44 << 31 << firstResult << outOfOne
                                        << "TODO: c++ type location" << -1 << -1;

    QTest::addRow("functionParameterICUsage")
            << file1Qml << 34 << 16 << firstResult << outOfOne << file1Qml << 7 << 15;

    QTest::addRow("ICBindingUsage")
            << file1Qml << 47 << 21 << firstResult << outOfOne << file1Qml << 7 << 15;
    QTest::addRow("ICBindingUsage2")
            << file1Qml << 49 << 11 << firstResult << outOfOne << file1Qml << 7 << 15;
    QTest::addRow("ICBindingUsage3")
            << file1Qml << 52 << 17 << firstResult << outOfOne << file1Qml << 7 << 15;
}

void tst_qmlls_utils::findTypeDefinitionFromLocation()
{
    QFETCH(QString, filePath);
    QFETCH(int, line);
    QFETCH(int, character);
    QFETCH(int, resultIndex);
    QFETCH(int, expectedItemsCount);
    QFETCH(QString, expectedFilePath);
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

    QQmlJS::Dom::DomCreationOptions options;
    options.setFlag(QQmlJS::Dom::DomCreationOption::WithSemanticAnalysis);
    options.setFlag(QQmlJS::Dom::DomCreationOption::WithScriptExpressions);

    auto [env, file] = createEnvironmentAndLoadFile(filePath, options);

    auto locations = QQmlLSUtils::itemsFromTextLocation(
            file.field(QQmlJS::Dom::Fields::currentItem), line - 1, character - 1);

    QCOMPARE(locations.size(), expectedItemsCount);

    auto base = QQmlLSUtils::findTypeDefinitionOf(locations[resultIndex].domItem);

    // if expectedFilePath is empty, we probably just want to make sure that it does
    // not crash
    if (expectedFilePath == noResultExpected) {
        QVERIFY(!base);
        return;
    }

    QEXPECT_FAIL("findIntProperty", "Builtins not supported yet", Abort);
    QEXPECT_FAIL("function-parameter-builtin", "Base types defined in C++ are not supported yet",
                 Abort);
    QEXPECT_FAIL("colorBinding", "Types from C++ bases not supported yet", Abort);
    QEXPECT_FAIL("rectangle-property", "Types from C++ bases not supported yet", Abort);
    QEXPECT_FAIL("icBase", "Base types defined in C++ are not supported yet", Abort);
    QVERIFY(base);

    auto fileObject =
            locations[resultIndex].domItem.goToFile(base->filename).as<QQmlJS::Dom::QmlFile>();

    // print some debug message when failing, instead of using QVERIFY2
    // (printing the type every time takes a lot of time).
    if constexpr (enable_debug_output) {
        if (!fileObject)
            qDebug() << "Could not find the file" << base->filename << "in the Dom.";
    }

    QVERIFY(fileObject);
    QCOMPARE(base->filename, expectedFilePath);
    QCOMPARE(fileObject->canonicalFilePath(), expectedFilePath);

    QCOMPARE(base->sourceLocation.startLine, quint32(expectedLine));
    QCOMPARE(base->sourceLocation.startColumn, quint32(expectedCharacter));
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

    QQmlJS::Dom::DomCreationOptions options;
    options.setFlag(QQmlJS::Dom::DomCreationOption::WithSemanticAnalysis);

    auto [env, file] = createEnvironmentAndLoadFile(filePath, options);

    // grab item using already tested QQmlLSUtils::findLastItemsContaining
    auto locations = QQmlLSUtils::itemsFromTextLocation(
            file.field(QQmlJS::Dom::Fields::currentItem), line - 1, character - 1);
    QCOMPARE(locations.size(), 1);

    // once the item is grabbed, make sure its line/character position can be obtained back
    auto t = QQmlJS::Dom::FileLocations::treeOf(locations.front().domItem);

    QCOMPARE(t->info().fullRegion.startLine, quint32(expectedLine));
    QCOMPARE(t->info().fullRegion.startColumn, quint32(expectedCharacter));
}

void tst_qmlls_utils::findBaseObject_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("character");
    // to avoid mixing up the types (because they are all called Item or QQuickItem eitherway)
    // mark them with properties and detect right types by their marker property,
    // usually called (in<Filename>DotQml or in<Inline component Name>)
    QTest::addColumn<QSet<QString>>("expectedPropertyName");
    // because types inherit properties, make sure that derived type properties are not in the base
    // type, to correctly detect mixups between types and their base types
    QTest::addColumn<QSet<QString>>("unExpectedPropertyName");

    // (non) Expected Properties Names = ePN (nEPN)
    // marker properties for the root object in BaseType.qml
    QSet<QString> ePNBaseType;
    ePNBaseType << u"inBaseTypeDotQml"_s;
    QSet<QString> nEPNBaseType;
    nEPNBaseType << u"inTypeDotQml"_s;

    // marker properties for the root object in Type.qml
    QSet<QString> ePNType;
    ePNType << u"inBaseTypeDotQml"_s << u"inTypeDotQml"_s;
    QSet<QString> nEPNType;

    // marker properties for QQuickItem (e.g. the base of "Item")
    QSet<QString> ePNQQuickItem;
    QSet<QString> nEPNQQuickItem;
    nEPNQQuickItem << u"inBaseTypeDotQml"_s << u"inTypeDotQml"_s;

    // marker properties for MyInlineComponent
    QSet<QString> ePNMyInlineComponent;
    QSet<QString> nEPNMyInlineComponent;
    ePNMyInlineComponent << u"inBaseTypeDotQml"_s << u"inTypeDotQml"_s << u"inMyInlineComponent"_s;

    // marker properties for MyNestedInlineComponent
    QSet<QString> ePNMyNestedInlineComponent;
    ePNMyNestedInlineComponent << u"inBaseTypeDotQml"_s << u"inTypeDotQml"_s
                               << u"inMyInlineComponent"_s;
    QSet<QString> nEPNMyNestedInlineComponent;
    nEPNMyNestedInlineComponent << u"inMyNestedInlineComponent"_s;

    const int rootElementDefLine = 6;
    QTest::addRow("root-element") << testFile(u"Type.qml"_s) << rootElementDefLine << 5
                                  << ePNQQuickItem << nEPNQQuickItem;
    QTest::addRow("root-element-from-id") << testFile(u"Type.qml"_s) << rootElementDefLine + 1 << 12
                                          << ePNBaseType << nEPNBaseType;

    const int myInlineComponentDefLine = 10;
    // on the component name: go to BaseType
    QTest::addRow("ic-name") << testFile(u"Type.qml"_s) << myInlineComponentDefLine << 26
                             << ePNBaseType << nEPNBaseType;
    // on the "BaseType" type: go to QQuickitem (base type of BaseType).
    QTest::addRow("ic-basetypename") << testFile(u"Type.qml"_s) << myInlineComponentDefLine << 37
                                     << ePNQQuickItem << nEPNQQuickItem;
    QTest::addRow("ic-from-id") << testFile(u"Type.qml"_s) << myInlineComponentDefLine + 1 << 19
                                << ePNBaseType << nEPNBaseType;

    const int inlineTypeDefLine = 15;
    QTest::addRow("inline") << testFile(u"Type.qml"_s) << inlineTypeDefLine << 23 << ePNQQuickItem
                            << nEPNQQuickItem;
    QTest::addRow("inline2") << testFile(u"Type.qml"_s) << inlineTypeDefLine << 38 << ePNQQuickItem
                             << nEPNQQuickItem;
    QTest::addRow("inline3") << testFile(u"Type.qml"_s) << inlineTypeDefLine << 15 << ePNQQuickItem
                             << nEPNQQuickItem;
    QTest::addRow("inline-from-id") << testFile(u"Type.qml"_s) << inlineTypeDefLine + 1 << 24
                                    << ePNBaseType << nEPNBaseType;

    const int inlineIcDefLine = 23;
    QTest::addRow("inline-ic") << testFile(u"Type.qml"_s) << inlineIcDefLine << 35
                               << ePNMyInlineComponent << nEPNMyInlineComponent;
    QTest::addRow("inline-ic-from-id") << testFile(u"Type.qml"_s) << inlineIcDefLine + 1 << 48
                                       << ePNMyInlineComponent << nEPNMyInlineComponent;

    const int inlineNestedIcDefLine = 27;
    QTest::addRow("inline-ic2") << testFile(u"Type.qml"_s) << inlineNestedIcDefLine << 22
                                << ePNMyNestedInlineComponent << nEPNMyNestedInlineComponent;
    QTest::addRow("inline-ic2-from-id")
            << testFile(u"Type.qml"_s) << inlineNestedIcDefLine << 22 << ePNMyNestedInlineComponent
            << nEPNMyNestedInlineComponent;
}

void tst_qmlls_utils::findBaseObject()
{
    const QByteArray failOnInlineComponentsMessage =
            "The Dom cannot resolve inline components from the basetype yet.";

    QFETCH(QString, filePath);
    QFETCH(int, line);
    QFETCH(int, character);
    QFETCH(QSet<QString>, expectedPropertyName);
    QFETCH(QSet<QString>, unExpectedPropertyName);

    // they all start at 1.
    Q_ASSERT(line > 0);
    Q_ASSERT(character > 0);

    QQmlJS::Dom::DomCreationOptions options;
    options.setFlag(QQmlJS::Dom::DomCreationOption::WithSemanticAnalysis);

    auto [env, file] = createEnvironmentAndLoadFile(filePath, options);

    // grab item using already tested QQmlLSUtils::findLastItemsContaining
    auto locations = QQmlLSUtils::itemsFromTextLocation(
            file.field(QQmlJS::Dom::Fields::currentItem), line - 1, character - 1);
    if constexpr (enable_debug_output) {
        if (locations.size() > 1) {
            for (auto &x : locations)
                qDebug() << x.domItem.toString();
        }
    }
    QCOMPARE(locations.size(), 1);

    auto typeLocation = QQmlLSUtils::findTypeDefinitionOf(locations.front().domItem);
    QEXPECT_FAIL("inline-ic", failOnInlineComponentsMessage, Abort);
    QEXPECT_FAIL("inline-ic2", failOnInlineComponentsMessage, Abort);
    QEXPECT_FAIL("inline-ic2-from-id", failOnInlineComponentsMessage, Abort);
    QVERIFY(typeLocation);
    QQmlJS::Dom::DomItem type = QQmlLSUtils::sourceLocationToDomItem(
            locations.front().domItem.goToFile(typeLocation->filename),
            typeLocation->sourceLocation);
    auto base = QQmlLSUtils::baseObject(type);

    QEXPECT_FAIL("inline-ic-from-id", failOnInlineComponentsMessage, Abort);

    if constexpr (enable_debug_output) {
        if (!base)
            qDebug() << u"Could not find the base of type "_s << type << u" from item:\n"_s
                     << locations.front().domItem.toString();
    }

    QVERIFY(base);

    const QSet<QString> propertyDefs = base.field(QQmlJS::Dom::Fields::propertyDefs).keys();
    expectedPropertyName.subtract(propertyDefs);
    QVERIFY2(expectedPropertyName.empty(),
             u"Incorrect baseType found: it is missing following marker properties: "_s
                     .append(printSet(expectedPropertyName))
                     .toLatin1());
    unExpectedPropertyName.intersect(propertyDefs);
    QVERIFY2(unExpectedPropertyName.empty(),
             u"Incorrect baseType found: it has an unexpected marker properties: "_s
                     .append(printSet(unExpectedPropertyName))
                     .toLatin1());
}

void tst_qmlls_utils::findUsages_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("character");
    QTest::addColumn<QList<QQmlLSUtilsLocation>>("expectedUsages");

    const QString testFileName = testFile(u"JSUsages.qml"_s);
    QString testFileContent;
    {
        QFile file(testFileName);
        QVERIFY(file.open(QIODeviceBase::ReadOnly));
        testFileContent = QString::fromUtf8(file.readAll());
    }

    QList<QQmlLSUtilsLocation> sumUsages{
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 8, 13, strlen("sum")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 10, 13, strlen("sum")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 10, 19, strlen("sum")),
    };

    QList<QQmlLSUtilsLocation> iUsages{
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 9, 17, strlen("i")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 9, 24, strlen("i")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 9, 32, strlen("i")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 9, 36, strlen("i")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 10, 25, strlen("i")),
    };

    QList<QQmlLSUtilsLocation> helloPropertyUsages{
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 17, 18, strlen("helloProperty")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 24, 13, strlen("helloProperty")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 24, 29, strlen("helloProperty")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 65, 60, strlen("helloProperty")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 151, 9,
                                  strlen("helloPropertyChanged")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 153, 5,
                                  strlen("onHelloPropertyChanged")),
    };

    QList<QQmlLSUtilsLocation> subItemHelloPropertyUsages{
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 32, 20, strlen("helloProperty")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 34, 25, strlen("helloProperty")),
    };

    QList<QQmlLSUtilsLocation> ICHelloPropertyUsages{
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 37, 22, strlen("helloProperty")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 39, 20, strlen("helloProperty")),
    };

    QList<QQmlLSUtilsLocation> p2Usages{
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 18, 18, strlen("p2")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 24, 55, strlen("p2")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 32, 36, strlen("p2")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 39, 36, strlen("p2")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 66, 31, strlen("p2")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 67, 37, strlen("p2")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 68, 43, strlen("p2")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 69, 49, strlen("p2")),
    };

    QList<QQmlLSUtilsLocation> nestedUsages{
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 62, 13, strlen("myNested")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 65, 17, strlen("myNested")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 66, 17, strlen("myNested")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 67, 17, strlen("myNested")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 68, 17, strlen("myNested")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 69, 17, strlen("myNested")),
    };

    QList<QQmlLSUtilsLocation> rootIdUsages{
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 81, 9, strlen("rootId")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 84, 20, strlen("rootId")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 108, 13, strlen("rootId")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 114, 13, strlen("rootId")),
    };

    QList<QQmlLSUtilsLocation> nestedComponent3Usages{
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 47, 35, strlen("inner")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 65, 32, strlen("inner")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 68, 32, strlen("inner")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 69, 32, strlen("inner")),
    };

    QList<QQmlLSUtilsLocation> nestedComponent3P2Usages{
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 68, 38, strlen("p2")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 53, 22, strlen("p2")),
    };

    QList<QQmlLSUtilsLocation> recursiveUsages{
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 72, 14, strlen("recursive")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 74, 24, strlen("recursive")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 74, 34, strlen("recursive")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 74, 51, strlen("recursive")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 74, 68, strlen("recursive")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 76, 20, strlen("recursive")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 79, 34, strlen("recursive")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 84, 27, strlen("recursive")),
    };

    QList<QQmlLSUtilsLocation> helloSignalUsages{
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 88, 12, strlen("helloSignal")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 91, 9, strlen("helloSignal")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 93, 13, strlen("helloSignal")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 97, 17, strlen("helloSignal")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 101, 9, strlen("helloSignal")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 119, 5, strlen("onHelloSignal")),
    };

    QList<QQmlLSUtilsLocation> widthChangedUsages{
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 103, 13, strlen("widthChanged")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 107, 17, strlen("widthChanged")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 108, 20, strlen("widthChanged")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 114, 20, strlen("widthChanged")),
    };

    QList<QQmlLSUtilsLocation> helloPropertyBindingUsages{
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 121, 18,
                                  strlen("helloPropertyBinding")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 122, 5,
                                  strlen("helloPropertyBinding")),
    };

    QList<QQmlLSUtilsLocation> myHelloHandlerUsages{
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 118, 14, strlen("myHelloHandler")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 119, 20, strlen("myHelloHandler")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 125, 29, strlen("myHelloHandler")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 126, 24, strlen("myHelloHandler")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 134, 17, strlen("myHelloHandler")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 135, 24, strlen("myHelloHandler")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 136, 21, strlen("myHelloHandler")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 144, 19, strlen("myHelloHandler")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 153, 29, strlen("myHelloHandler")),
    };

    QList<QQmlLSUtilsLocation> checkHandlersUsages {
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 124, 18, strlen("checkHandlers")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 125, 5,
                                  strlen("onCheckHandlersChanged")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 128, 9,
                                  strlen("checkHandlersChanged")),
    };

    QList<QQmlLSUtilsLocation> checkCppHandlersUsages {
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 126, 5,
                                  strlen("onChildrenChanged")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 129, 9, strlen("childrenChanged")),
    };
    QList<QQmlLSUtilsLocation> checkHandlersUsages2 {
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 131, 18, strlen("_")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 134, 5, strlen("on_Changed")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 138, 9, strlen("_Changed")),
    };
    QList<QQmlLSUtilsLocation> checkHandlersUsages3 {
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 132, 18, strlen("______42")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 135, 5,
                                  strlen("on______42Changed")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 139, 9, strlen("______42Changed")),
    };
    QList<QQmlLSUtilsLocation> checkHandlersUsages4 {
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 133, 18, strlen("_123a")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 136, 5,  strlen("on_123AChanged")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 140, 9, strlen("_123aChanged")),
    };

    QList<QQmlLSUtilsLocation> mouseArea1ClickedUsages{
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 159, 9, strlen("onClicked")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 167, 23, strlen("clicked")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 183, 15, strlen("clicked")),
    };

    QList<QQmlLSUtilsLocation> mouseArea2ClickedUsages{
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 166, 22, strlen("onClicked")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 184, 15, strlen("clicked")),
    };

    QList<QQmlLSUtilsLocation> mouseArea3ClickedUsages{
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 168, 23, strlen("clicked")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 178, 9, strlen("onClicked")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 185, 15, strlen("clicked")),
    };

    QList<QQmlLSUtilsLocation> aParamUsages{
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 188, 30, strlen("a")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 189, 16, strlen("a")),
    };
    QList<QQmlLSUtilsLocation> bParamUsages{
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 188, 38, strlen("b")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 189, 20, strlen("b")),
    };
    QList<QQmlLSUtilsLocation> cParamUsages{
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 188, 49, strlen("c")),
        QQmlLSUtilsLocation::from(testFileName, testFileContent, 189, 24, strlen("c")),
    };

    std::sort(sumUsages.begin(), sumUsages.end());
    std::sort(iUsages.begin(), iUsages.end());
    std::sort(subItemHelloPropertyUsages.begin(), subItemHelloPropertyUsages.end());
    std::sort(ICHelloPropertyUsages.begin(), ICHelloPropertyUsages.end());
    std::sort(p2Usages.begin(), p2Usages.end());
    std::sort(nestedUsages.begin(), nestedUsages.end());
    std::sort(nestedComponent3Usages.begin(), nestedComponent3Usages.end());
    std::sort(nestedComponent3P2Usages.begin(), nestedComponent3P2Usages.end());
    std::sort(recursiveUsages.begin(), recursiveUsages.end());
    std::sort(helloSignalUsages.begin(), helloSignalUsages.end());
    std::sort(widthChangedUsages.begin(), widthChangedUsages.end());
    std::sort(helloPropertyBindingUsages.begin(), helloPropertyBindingUsages.end());
    std::sort(myHelloHandlerUsages.begin(), myHelloHandlerUsages.end());
    std::sort(checkHandlersUsages.begin(), checkHandlersUsages.end());
    std::sort(checkCppHandlersUsages.begin(), checkCppHandlersUsages.end());
    std::sort(checkHandlersUsages2.begin(), checkHandlersUsages2.end());
    std::sort(checkHandlersUsages3.begin(), checkHandlersUsages3.end());
    std::sort(checkHandlersUsages4.begin(), checkHandlersUsages4.end());
    std::sort(mouseArea1ClickedUsages.begin(), mouseArea1ClickedUsages.end());
    std::sort(mouseArea2ClickedUsages.begin(), mouseArea2ClickedUsages.end());
    std::sort(mouseArea3ClickedUsages.begin(), mouseArea3ClickedUsages.end());
    std::sort(aParamUsages.begin(), aParamUsages.end());
    std::sort(bParamUsages.begin(), bParamUsages.end());
    std::sort(cParamUsages.begin(), cParamUsages.end());

    QTest::addRow("findSumFromDeclaration") << testFileName << 8 << 13 << sumUsages;
    QTest::addRow("findSumFromUsage") << testFileName << 10 << 20 << sumUsages;

    QTest::addRow("findIFromDeclaration") << testFileName << 9 << 17 << iUsages;
    QTest::addRow("findIFromUsage") << testFileName << 9 << 24 << iUsages;
    QTest::addRow("findIFromUsage2") << testFileName << 10 << 25 << iUsages;

    QTest::addRow("findPropertyFromDeclaration") << testFileName << 17 << 18 << helloPropertyUsages;
    QTest::addRow("findPropertyFromDeclaration2") << testFileName << 18 << 18 << p2Usages;
    QTest::addRow("findPropertyFromDeclarationInSubItem")
            << testFileName << 34 << 29 << subItemHelloPropertyUsages;
    QTest::addRow("findPropertyFromDeclarationInIC")
            << testFileName << 37 << 22 << ICHelloPropertyUsages;

    QTest::addRow("findPropertyFromUsage") << testFileName << 24 << 13 << helloPropertyUsages;
    QTest::addRow("findPropertyFromUsage2") << testFileName << 24 << 36 << helloPropertyUsages;
    QTest::addRow("findPropertyFromUsageInSubItem")
            << testFileName << 32 << 26 << subItemHelloPropertyUsages;
    QTest::addRow("findPropertyFromUsageInIC") << testFileName << 39 << 20 << ICHelloPropertyUsages;

    QTest::addRow("findIdFromUsage") << testFileName << 67 << 20 << nestedUsages;
    QTest::addRow("findIdFromDefinition") << testFileName << 62 << 17 << nestedUsages;
    QTest::addRow("findIdFromUsageInChild") << testFileName << 84 << 22 << rootIdUsages;

    QTest::addRow("findPropertyFromUsageInFieldMemberExpression")
            << testFileName << 69 << 34 << nestedComponent3Usages;

    QTest::addRow("findFieldMemberExpressionUsageFromPropertyDefinition")
            << testFileName << 47 << 38 << nestedComponent3Usages;

    QTest::addRow("findProperty2FromUsageInFieldMemberExpression")
            << testFileName << 68 << 39 << nestedComponent3P2Usages;

    QTest::addRow("findFunctionUsage") << testFileName << 74 << 30 << recursiveUsages;
    QTest::addRow("findFunctionUsage2") << testFileName << 76 << 24 << recursiveUsages;
    QTest::addRow("findQualifiedFunctionUsage") << testFileName << 84 << 31 << recursiveUsages;
    QTest::addRow("findFunctionUsageFromDefinition") << testFileName << 72 << 17 << recursiveUsages;
    QTest::addRow("findJSMethodFromUsageInBinding")
            << testFileName << 119 << 27 << myHelloHandlerUsages;
    QTest::addRow("findJSMethodFromDefinition")
            << testFileName << 118 << 22 << myHelloHandlerUsages;
    QTest::addRow("findJSMethodFromDefinition2")
            << testFileName << 118 << 9 << myHelloHandlerUsages;

    QTest::addRow("findQmlSignalUsageFromDefinition")
            << testFileName << 88 << 17 << helloSignalUsages;
    QTest::addRow("findQmlSignalUsageFromUsage") << testFileName << 93 << 17 << helloSignalUsages;
    QTest::addRow("findQmlSignalUsageFromHandler")
            << testFileName << 119 << 11 << helloSignalUsages;

    QTest::addRow("findCppSignalUsageFromUsage") << testFileName << 107 << 23 << widthChangedUsages;
    QTest::addRow("findCppSignalUsageFromQualifiedUsage")
            << testFileName << 108 << 23 << widthChangedUsages;
    QTest::addRow("findCppSignalUsageFromQualifiedUsage2")
            << testFileName << 114 << 24 << widthChangedUsages;

    QTest::addRow("findBindingUsagesFromDefinition")
            << testFileName << 121 << 21 << helloPropertyBindingUsages;
    QTest::addRow("findBindingUsagesFromBinding")
            << testFileName << 122 << 19 << helloPropertyBindingUsages;

    QTest::addRow("findQmlPropertyHandlerFromDefinition")
            << testFileName << 124 << 18 << checkHandlersUsages;
    QTest::addRow("findQmlPropertyHandlerFromHandler")
            << testFileName << 125 << 5 << checkHandlersUsages;
    QTest::addRow("findQmlPropertyHandlerFromSignalCall")
            << testFileName << 128 << 9 << checkHandlersUsages;

    QTest::addRow("findCppPropertyHandlerFromHandler")
            << testFileName << 126 << 5 << checkCppHandlersUsages;
    QTest::addRow("findCppPropertyHandlerFromSignalCall")
            << testFileName << 129 << 9 << checkCppHandlersUsages;

    QTest::addRow("findQmlPropertyHandler2FromDefinition")
            << testFileName << 131 << 18 << checkHandlersUsages2;
    QTest::addRow("findQmlPropertyHandler3FromDefinition")
            << testFileName << 132 << 18 << checkHandlersUsages3;
    QTest::addRow("findQmlPropertyHandler4FromDefinition")
            << testFileName << 133 << 18 << checkHandlersUsages4;

    QTest::addRow("findSignalsInConnectionObject")
            << testFileName << 183 << 15 << mouseArea1ClickedUsages;
    QTest::addRow("findSignalsInConnectionObjectWithNoTarget")
            << testFileName << 184 << 15 << mouseArea2ClickedUsages;
    QTest::addRow("findSignalsInConnectionObjectWithTarget")
            << testFileName << 185 << 15 << mouseArea3ClickedUsages;

    QTest::addRow("findMethodParameter") << testFileName << 189 << 16 << aParamUsages;
    QTest::addRow("findMethodParameter2") << testFileName << 188 << 30 << aParamUsages;
}

void tst_qmlls_utils::findUsages()
{
    QFETCH(QString, filePath);
    QFETCH(int, line);
    QFETCH(int, character);
    QFETCH(QList<QQmlLSUtilsLocation>, expectedUsages);
    QVERIFY(std::is_sorted(expectedUsages.begin(), expectedUsages.end()));

    QQmlJS::Dom::DomCreationOptions options;
    options.setFlag(QQmlJS::Dom::DomCreationOption::WithSemanticAnalysis);
    options.setFlag(QQmlJS::Dom::DomCreationOption::WithScriptExpressions);

    auto [env, file] = createEnvironmentAndLoadFile(filePath, options);

    auto locations = QQmlLSUtils::itemsFromTextLocation(
            file.field(QQmlJS::Dom::Fields::currentItem), line - 1, character - 1);

    if constexpr (enable_debug_output) {
        if (locations.size() > 1) {
            for (auto &x : locations)
                qDebug() << x.domItem.toString();
        }
    }
    QCOMPARE(locations.size(), 1);

    auto usages = QQmlLSUtils::findUsagesOf(locations.front().domItem);

    if constexpr (enable_debug_output) {
        if (usages != expectedUsages) {
            qDebug() << "Got:\n";
            for (auto &x : usages) {
                qDebug() << x.filename << "(" << x.sourceLocation.startLine << ", "
                         << x.sourceLocation.startColumn << "), " << x.sourceLocation.offset << "+"
                         << x.sourceLocation.length;
            }
            qDebug() << "But expected: \n";
            for (auto &x : expectedUsages) {
                qDebug() << x.filename << "(" << x.sourceLocation.startLine << ", "
                         << x.sourceLocation.startColumn << "), " << x.sourceLocation.offset << "+"
                         << x.sourceLocation.length;
            }
        }
    }
    QCOMPARE(usages, expectedUsages);
}

void tst_qmlls_utils::renameUsages_data()
{
    QTest::addColumn<QString>("filePath");
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("character");
    QTest::addColumn<QString>("newName");
    QTest::addColumn<QList<QQmlLSUtilsEdit>>("expectedRenames");
    QTest::addColumn<QString>("expectedError");

    const QString testFileName = testFile(u"JSUsages.qml"_s);
    QString testFileContent;
    {
        QFile file(testFileName);
        QVERIFY(file.open(QIODeviceBase::ReadOnly));
        testFileContent = QString::fromUtf8(file.readAll());
    }

    const QString noError;
    const QList<QQmlLSUtilsEdit> noRenames;

    QList<QQmlLSUtilsEdit> methodFRename{
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 72, 14, strlen("recursive"),
                              u"newNameNewMe"_s),
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 74, 24, strlen("recursive"),
                              u"newNameNewMe"_s),
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 74, 34, strlen("recursive"),
                              u"newNameNewMe"_s),
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 74, 51, strlen("recursive"),
                              u"newNameNewMe"_s),
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 74, 68, strlen("recursive"),
                              u"newNameNewMe"_s),
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 76, 20, strlen("recursive"),
                              u"newNameNewMe"_s),
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 79, 34, strlen("recursive"),
                              u"newNameNewMe"_s),
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 84, 27, strlen("recursive"),
                              u"newNameNewMe"_s),
    };

    QList<QQmlLSUtilsEdit> JSIdentifierSumRename{
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 8, 13, strlen("sum"),
                              u"sumsumsum123"_s),
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 10, 13, strlen("sum"),
                              u"sumsumsum123"_s),
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 10, 19, strlen("sum"),
                              u"sumsumsum123"_s),
    };

    QList<QQmlLSUtilsEdit> qmlSignalRename{
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 88, 12, strlen("helloSignal"),
                              u"finalSignal"_s),
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 91, 9, strlen("helloSignal"),
                              u"finalSignal"_s),
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 93, 13, strlen("helloSignal"),
                              u"finalSignal"_s),
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 97, 17, strlen("helloSignal"),
                              u"finalSignal"_s),
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 101, 9, strlen("helloSignal"),
                              u"finalSignal"_s),
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 119, 5, strlen("onHelloSignal"),
                              u"onFinalSignal"_s),
    };

    QList<QQmlLSUtilsEdit> helloPropertyRename{
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 17, 18, strlen("helloProperty"),
                              u"freshPropertyName"_s),
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 24, 13, strlen("helloProperty"),
                              u"freshPropertyName"_s),
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 24, 29, strlen("helloProperty"),
                              u"freshPropertyName"_s),
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 65, 60, strlen("helloProperty"),
                              u"freshPropertyName"_s),
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 151, 9, strlen("helloPropertyChanged"),
                              u"freshPropertyNameChanged"_s),
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 153, 5,
                              strlen("onHelloPropertyChanged"), u"onFreshPropertyNameChanged"_s),
    };

    QList<QQmlLSUtilsEdit> nestedComponentRename{
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 42, 15, strlen("NestedComponent"),
                              u"SuperInlineComponent"_s),
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 61, 5, strlen("NestedComponent"),
                              u"SuperInlineComponent"_s),
    };

    QList<QQmlLSUtilsEdit> myNestedIdRename{
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 62, 13, strlen("myNested"),
                              u"freshNewIdForMyNested"_s),
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 65, 17, strlen("myNested"),
                              u"freshNewIdForMyNested"_s),
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 66, 17, strlen("myNested"),
                              u"freshNewIdForMyNested"_s),
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 67, 17, strlen("myNested"),
                              u"freshNewIdForMyNested"_s),
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 68, 17, strlen("myNested"),
                              u"freshNewIdForMyNested"_s),
        QQmlLSUtilsEdit::from(testFileName, testFileContent, 69, 17, strlen("myNested"),
                              u"freshNewIdForMyNested"_s),
    };

    std::sort(methodFRename.begin(), methodFRename.end());
    std::sort(JSIdentifierSumRename.begin(), JSIdentifierSumRename.end());
    std::sort(qmlSignalRename.begin(), qmlSignalRename.end());
    std::sort(helloPropertyRename.begin(), helloPropertyRename.end());
    std::sort(helloPropertyRename.begin(), helloPropertyRename.end());
    std::sort(nestedComponentRename.begin(), nestedComponentRename.end());
    std::sort(myNestedIdRename.begin(), myNestedIdRename.end());

    const QString parserError = u"Invalid EcmaScript identifier!"_s;

    QTest::addRow("renameMethod") << testFileName << 72 << 19 << u"newNameNewMe"_s << methodFRename
                                  << noError;
    QTest::addRow("renameJSIdentifier")
            << testFileName << 10 << 19 << u"sumsumsum123"_s << JSIdentifierSumRename << noError;
    QTest::addRow("renameQmlSignal")
            << testFileName << 93 << 19 << u"finalSignal"_s << qmlSignalRename << noError;
    QTest::addRow("renameQmlSignalHandler")
            << testFileName << 119 << 10 << u"onFinalSignal"_s << qmlSignalRename << noError;

    QTest::addRow("renameQmlProperty")
            << testFileName << 17 << 20 << u"freshPropertyName"_s << helloPropertyRename << noError;
    QTest::addRow("renameQmlPropertyChanged")
            << testFileName << 151 << 18 << u"freshPropertyNameChanged"_s << helloPropertyRename
            << noError;
    QTest::addRow("renameQmlPropertyChangedHandler")
            << testFileName << 153 << 22 << u"onFreshPropertyNameChanged"_s << helloPropertyRename
            << noError;

    QTest::addRow("renameQmlObjectId") << testFileName << 65 << 21 << u"freshNewIdForMyNested"_s
                                       << myNestedIdRename << noError;

    // rename forbidden stuff
    QTest::addRow("renameCPPDefinedItem") << testFileName << 144 << 13 << u"onHelloWorld"_s
                                          << noRenames << u"defined in non-QML files."_s;
    QTest::addRow("renameFunctionKeyword") << testFileName << 8 << 10 << u"HelloWorld"_s
                                           << noRenames << "Requested item cannot be renamed";
    QTest::addRow("invalidCharactersInIdentifier")
            << testFileName << 12 << 22 << u"\""_s << noRenames << parserError;
    QTest::addRow("invalidCharactersInIdentifier2")
            << testFileName << 12 << 22 << u"hello world"_s << noRenames << parserError;
    QTest::addRow("invalidCharactersInIdentifier3")
            << testFileName << 12 << 22 << u"sum.sum.sum"_s << noRenames << parserError;
    QTest::addRow("emptyIdentifier")
            << testFileName << 12 << 22 << QString() << noRenames << parserError;
    QTest::addRow("usingKeywordAsIdentifier")
            << testFileName << 12 << 22 << u"function"_s << noRenames << parserError;

    QTest::addRow("changedSignalHandlerMissingOnChanged")
            << testFileName << 134 << 9 << u"___"_s << noRenames
            << u"Invalid name for a property changed handler identifier"_s;
    QTest::addRow("changedSignalHandlerMissingChanged")
            << testFileName << 134 << 9 << u"on___"_s << noRenames
            << u"Invalid name for a property changed handler identifier"_s;
    QTest::addRow("changedSignalHandlerMissingOn")
            << testFileName << 134 << 9 << u"___Changed"_s << noRenames
            << u"Invalid name for a property changed handler identifier"_s;
    QTest::addRow("changedSignalHandlerTypoInChanged")
            << testFileName << 134 << 9 << u"on___Chänged"_s << noRenames
            << u"Invalid name for a property changed handler identifier"_s;

    QTest::addRow("signalHandlerMissingOn")
            << testFileName << 119 << 10 << u"helloSuperSignal"_s << noRenames
            << u"Invalid name for a signal handler identifier"_s;
    QTest::addRow("signalHandlerMissingCapitalization")
            << testFileName << 119 << 10 << u"onhelloSuperSignal"_s << noRenames
            << u"Invalid name for a signal handler identifier"_s;

    QTest::addRow("JSIdentifierStartsWithNumber")
            << testFileName << 67 << 13 << u"123"_s << noRenames << parserError;
}

void tst_qmlls_utils::renameUsages()
{
    // findAndRenameUsages() already tests if all usages will be renamed
    // now test that the new name is correctly passed
    QFETCH(QString, filePath);
    QFETCH(int, line);
    QFETCH(int, character);
    QFETCH(QString, newName);
    QFETCH(QList<QQmlLSUtilsEdit>, expectedRenames);
    QFETCH(QString, expectedError);

    QVERIFY(std::is_sorted(expectedRenames.begin(), expectedRenames.end()));

    QQmlJS::Dom::DomCreationOptions options;
    options.setFlag(QQmlJS::Dom::DomCreationOption::WithSemanticAnalysis);
    options.setFlag(QQmlJS::Dom::DomCreationOption::WithScriptExpressions);

    auto [env, file] = createEnvironmentAndLoadFile(filePath, options);

    auto locations = QQmlLSUtils::itemsFromTextLocation(
            file.field(QQmlJS::Dom::Fields::currentItem), line - 1, character - 1);

    if constexpr (enable_debug_output) {
        if (locations.size() > 1) {
            for (auto &x : locations)
                qDebug() << x.domItem.toString();
        }
    }
    QCOMPARE(locations.size(), 1);

    if (auto errors = QQmlLSUtils::checkNameForRename(locations.front().domItem, newName)) {
        if constexpr (enable_debug_output) {
            if (expectedError.isEmpty())
                qDebug() << "Expected no error but got" << errors->message;
            if (!errors->message.contains(expectedError))
                qDebug() << "Cannot find" << expectedError << "in" << errors->message;
        }
        QVERIFY(!expectedError.isEmpty());
        QVERIFY(errors->message.contains(expectedError));
        return;
    }
    auto edits = QQmlLSUtils::renameUsagesOf(locations.front().domItem, newName);

    if constexpr (enable_debug_output) {
        if (edits != expectedRenames) {
            qDebug() << "Got:\n";
            for (auto &x : edits) {
                qDebug() << x.replacement << x.location.filename << "("
                         << x.location.sourceLocation.startLine << ", "
                         << x.location.sourceLocation.startColumn << "), "
                         << x.location.sourceLocation.offset << "+"
                         << x.location.sourceLocation.length;
            }
            qDebug() << "But expected: \n";
            for (auto &x : expectedRenames) {
                qDebug() << x.replacement << x.location.filename << "("
                         << x.location.sourceLocation.startLine << ", "
                         << x.location.sourceLocation.startColumn << "), "
                         << x.location.sourceLocation.offset << "+"
                         << x.location.sourceLocation.length;
            }
        }
    }
    QCOMPARE(edits, expectedRenames);
}

void tst_qmlls_utils::findDefinitionFromLocation_data()
{
    QTest::addColumn<QString>("filePath");
    // keep in mind that line and character are starting at 1!
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("character");

    QTest::addColumn<QString>("expectedFilePath");
    // set to -1 when unchanged from above line and character. 0-based.
    QTest::addColumn<int>("expectedLine");
    QTest::addColumn<int>("expectedCharacter");
    QTest::addColumn<size_t>("expectedLength");

    const QString JSDefinitionsQml = testFile(u"JSDefinitions.qml"_s);
    const QString BaseTypeQml = testFile(u"BaseType.qml"_s);

    QTest::addRow("JSIdentifierX")
            << JSDefinitionsQml << 14 << 11 << JSDefinitionsQml << 13 << 13 << strlen("x");
    QTest::addRow("JSIdentifierX2")
            << JSDefinitionsQml << 15 << 11 << JSDefinitionsQml << 13 << 13 << strlen("x");
    QTest::addRow("propertyI") << JSDefinitionsQml << 14 << 14 << JSDefinitionsQml << 9 << 18
                               << strlen("i");
    QTest::addRow("qualifiedPropertyI")
            << JSDefinitionsQml << 15 << 21 << JSDefinitionsQml << 9 << 18 << strlen("i");
    QTest::addRow("inlineComponentProperty")
            << JSDefinitionsQml << 62 << 21 << JSDefinitionsQml << 54 << 22 << strlen("data");

    QTest::addRow("parameterA") << JSDefinitionsQml << 10 << 16 << JSDefinitionsQml << 10 << 16
                                << strlen("a");
    QTest::addRow("parameterAUsage")
            << JSDefinitionsQml << 10 << 39 << JSDefinitionsQml << -1 << 16 << strlen("a");

    QTest::addRow("parameterB") << JSDefinitionsQml << 10 << 28 << JSDefinitionsQml << 10 << 28
                                << strlen("b");
    QTest::addRow("parameterBUsage")
            << JSDefinitionsQml << 10 << 86 << JSDefinitionsQml << -1 << 28 << strlen("b");

    QTest::addRow("comment") << JSDefinitionsQml << 10 << 21 << noResultExpected << -1 << -1
                             << size_t{};

    QTest::addRow("scopedX") << JSDefinitionsQml << 22 << 18 << JSDefinitionsQml << 21 << 17
                             << strlen("scoped");
    QTest::addRow("scopedX2") << JSDefinitionsQml << 25 << 22 << JSDefinitionsQml << 21 << 17
                              << strlen("scoped");
    QTest::addRow("scopedX3") << JSDefinitionsQml << 28 << 14 << JSDefinitionsQml << 19 << 13
                              << strlen("scoped");

    QTest::addRow("normalI") << JSDefinitionsQml << 22 << 23 << JSDefinitionsQml << 9 << 18
                             << strlen("i");
    QTest::addRow("scopedI") << JSDefinitionsQml << 25 << 27 << JSDefinitionsQml << 24 << 32
                             << strlen("i");

    QTest::addRow("shadowingProperty")
            << JSDefinitionsQml << 37 << 21 << JSDefinitionsQml << 34 << 22 << strlen("i");
    QTest::addRow("shadowingQualifiedProperty")
            << JSDefinitionsQml << 37 << 35 << JSDefinitionsQml << 34 << 22 << strlen("i");
    QTest::addRow("shadowedProperty")
            << JSDefinitionsQml << 37 << 49 << JSDefinitionsQml << 9 << 18 << strlen("i");

    QTest::addRow("propertyInBinding")
            << JSDefinitionsQml << 64 << 37 << JSDefinitionsQml << 9 << 18 << strlen("i");
    QTest::addRow("propertyInBinding2")
            << JSDefinitionsQml << 65 << 38 << JSDefinitionsQml << 9 << 18 << strlen("i");
    QTest::addRow("propertyInBinding3")
            << JSDefinitionsQml << 66 << 51 << JSDefinitionsQml << 9 << 18 << strlen("i");

    QTest::addRow("propertyFromDifferentFile")
            << JSDefinitionsQml << 72 << 20 << BaseTypeQml << 24 << 18 << strlen("helloProperty");

    QTest::addRow("id") << JSDefinitionsQml << 15 << 17 << JSDefinitionsQml << 7 << 9
                        << strlen("rootId");
    QTest::addRow("onId") << JSDefinitionsQml << 32 << 16 << JSDefinitionsQml << 32 << 13
                          << strlen("nested");
    QTest::addRow("parentId") << JSDefinitionsQml << 37 << 44 << JSDefinitionsQml << 7 << 9
                              << strlen("rootId");
    QTest::addRow("currentId") << JSDefinitionsQml << 37 << 30 << JSDefinitionsQml << 32 << 13
                               << strlen("nested");
    QTest::addRow("inlineComponentId")
            << JSDefinitionsQml << 56 << 35 << JSDefinitionsQml << 52 << 13 << strlen("helloIC");

    QTest::addRow("recursiveFunction")
            << JSDefinitionsQml << 39 << 28 << JSDefinitionsQml << 36 << 18 << strlen("f");
    QTest::addRow("recursiveFunction2")
            << JSDefinitionsQml << 39 << 39 << JSDefinitionsQml << 36 << 18 << strlen("f");
    QTest::addRow("functionFromFunction")
            << JSDefinitionsQml << 44 << 20 << JSDefinitionsQml << 36 << 18 << strlen("f");
    QTest::addRow("qualifiedFunctionName")
            << JSDefinitionsQml << 48 << 23 << JSDefinitionsQml << 36 << 18 << strlen("f");

    QTest::addRow("functionInParent")
            << JSDefinitionsQml << 44 << 37 << JSDefinitionsQml << 18 << 14 << strlen("ffff");
    QTest::addRow("functionFromDifferentFile")
            << JSDefinitionsQml << 72 << 47 << BaseTypeQml << 25 << 14 << strlen("helloFunction");
}

void tst_qmlls_utils::findDefinitionFromLocation()
{
    QFETCH(QString, filePath);
    QFETCH(int, line);
    QFETCH(int, character);
    QFETCH(QString, expectedFilePath);
    QFETCH(int, expectedLine);
    QFETCH(int, expectedCharacter);
    QFETCH(size_t, expectedLength);

    if (expectedLine == -1)
        expectedLine = line;
    if (expectedCharacter == -1)
        expectedCharacter = character;

    // they all start at 1.
    Q_ASSERT(line > 0);
    Q_ASSERT(character > 0);
    Q_ASSERT(expectedLine > 0);
    Q_ASSERT(expectedCharacter > 0);

    QQmlJS::Dom::DomCreationOptions options;
    options.setFlag(QQmlJS::Dom::DomCreationOption::WithSemanticAnalysis);
    options.setFlag(QQmlJS::Dom::DomCreationOption::WithScriptExpressions);

    auto [env, file] = createEnvironmentAndLoadFile(filePath, options);

    auto locations = QQmlLSUtils::itemsFromTextLocation(
            file.field(QQmlJS::Dom::Fields::currentItem), line - 1, character - 1);

    QCOMPARE(locations.size(), 1);

    auto definition = QQmlLSUtils::findDefinitionOf(locations.front().domItem);

    // if expectedFilePath is empty, we probably just want to make sure that it does
    // not crash
    if (expectedFilePath == noResultExpected) {
        QVERIFY(!definition);
        return;
    }

    QVERIFY(definition);

    QCOMPARE(definition->filename, expectedFilePath);

    QCOMPARE(definition->sourceLocation.startLine, quint32(expectedLine));
    QCOMPARE(definition->sourceLocation.startColumn, quint32(expectedCharacter));
    QCOMPARE(definition->sourceLocation.length, quint32(expectedLength));
}

void tst_qmlls_utils::resolveExpressionType_data()
{
    QTest::addColumn<QString>("filePath");
    // keep in mind that line and character are starting at 1!
    QTest::addColumn<int>("line");
    QTest::addColumn<int>("character");
    QTest::addColumn<QString>("expectedFile");
    // startline of the owners definition
    QTest::addColumn<int>("expectedLine");

    {
        const QString JSDefinitionsQml = testFile(u"JSDefinitions.qml"_s);
        const int parentLine = 6;
        const int childLine = 31;

        QTest::addRow("id") << JSDefinitionsQml << 15 << 17 << JSDefinitionsQml << parentLine;
        QTest::addRow("childIddInChild")
                << JSDefinitionsQml << 37 << 30 << JSDefinitionsQml << childLine;
        QTest::addRow("parentIdInChild")
                << JSDefinitionsQml << 37 << 43 << JSDefinitionsQml << parentLine;

        QTest::addRow("propertyI")
                << JSDefinitionsQml << 14 << 14 << JSDefinitionsQml << parentLine;
        QTest::addRow("qualifiedPropertyI")
                << JSDefinitionsQml << 15 << 21 << JSDefinitionsQml << parentLine;
        QTest::addRow("propertyIInChild")
                << JSDefinitionsQml << 37 << 21 << JSDefinitionsQml << childLine;
        QTest::addRow("qualifiedChildPropertyIInChild")
                << JSDefinitionsQml << 37 << 35 << JSDefinitionsQml << childLine;
        QTest::addRow("qualifiedParentPropertyIInChild")
                << JSDefinitionsQml << 37 << 49 << JSDefinitionsQml << parentLine;

        QTest::addRow("childMethod")
                << JSDefinitionsQml << 48 << 23 << JSDefinitionsQml << childLine;
        QTest::addRow("childMethod2")
                << JSDefinitionsQml << 44 << 20 << JSDefinitionsQml << childLine;
        QTest::addRow("parentMethod")
                << JSDefinitionsQml << 14 << 9 << JSDefinitionsQml << parentLine;
    }

    {
        const QString JSUsagesQml = testFile(u"JSUsages.qml"_s);
        const int rootLine = 6;
        const int nestedComponent2Line = 46;
        const int nestedComponent3Line = 51;
        const int nestedComponent4Line = 57;
        QTest::addRow("propertyAccess:inner.inner")
                << JSUsagesQml << 68 << 34 << JSUsagesQml << nestedComponent2Line;
        QTest::addRow("propertyAccess:inner.inner2")
                << JSUsagesQml << 69 << 34 << JSUsagesQml << nestedComponent2Line;
        QTest::addRow("propertyAccess:inner.inner.inner")
                << JSUsagesQml << 69 << 40 << JSUsagesQml << nestedComponent3Line;
        QTest::addRow("propertyAccess:inner.inner.inner.p2")
                << JSUsagesQml << 69 << 44 << JSUsagesQml << nestedComponent4Line;

        QTest::addRow("propertyAccess:helloProperty")
                << JSUsagesQml << 65 << 68 << JSUsagesQml << rootLine;
        QTest::addRow("propertyAccess:nestedHelloProperty")
                << JSUsagesQml << 65 << 46 << JSUsagesQml << nestedComponent4Line;
    }
}

void tst_qmlls_utils::resolveExpressionType()
{
    QFETCH(QString, filePath);
    QFETCH(int, line);
    QFETCH(int, character);
    QFETCH(QString, expectedFile);
    QFETCH(int, expectedLine);

    // they all start at 1.
    Q_ASSERT(line > 0);
    Q_ASSERT(character > 0);

    QQmlJS::Dom::DomCreationOptions options;
    options.setFlag(QQmlJS::Dom::DomCreationOption::WithSemanticAnalysis);
    options.setFlag(QQmlJS::Dom::DomCreationOption::WithScriptExpressions);

    auto [env, file] = createEnvironmentAndLoadFile(filePath, options);

    auto locations = QQmlLSUtils::itemsFromTextLocation(
            file.field(QQmlJS::Dom::Fields::currentItem), line - 1, character - 1);

    QCOMPARE(locations.size(), 1);

    auto definition = QQmlLSUtils::resolveExpressionType(
            locations.front().domItem, QQmlLSUtilsResolveOptions::ResolveOwnerType);

    QVERIFY(definition);
    QCOMPARE(definition->semanticScope->filePath(), expectedFile);
    QQmlJS::SourceLocation location = definition->semanticScope->sourceLocation();
    QCOMPARE((int)location.startLine, expectedLine);
}

void tst_qmlls_utils::isValidEcmaScriptIdentifier_data()
{
    QTest::addColumn<QString>("identifier");
    QTest::addColumn<bool>("isValid");

    QTest::addRow("f") << u"f"_s << true;
    QTest::addRow("f-unicode") << u"\\u0046"_s << true;
    QTest::addRow("starts-with-digit") << u"8helloWorld"_s << false;
    QTest::addRow("starts-with-unicode-digit") << u"\\u0038helloWorld"_s << false; // \u0038 == '8'
    QTest::addRow("keyword") << u"return"_s << false;
    QTest::addRow("not-keyword") << u"returny"_s << true;
}

void tst_qmlls_utils::isValidEcmaScriptIdentifier()
{
    QFETCH(QString, identifier);
    QFETCH(bool, isValid);

    QCOMPARE(QQmlLSUtils::isValidEcmaScriptIdentifier(identifier), isValid);
}

QTEST_MAIN(tst_qmlls_utils)
