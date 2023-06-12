// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tst_qmlls_utils.h"

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
constexpr static bool enable_debug_output = false;

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
    QTest::addColumn<QString>("expectedTypeName");
    QTest::addColumn<QString>("expectedFilePath");
    // set to -1 when unchanged from above line and character. 0-based.
    QTest::addColumn<int>("expectedLine");
    QTest::addColumn<int>("expectedCharacter");

    const QString file1Qml = testFile(u"file1.qml"_s);
    const QString TypeQml = testFile(u"Type.qml"_s);
    // pass this as file when no result is expected, e.g. for type definition of "var".

    QTest::addRow("onCProperty") << file1Qml << 11 << 16 << firstResult << outOfOne << "file1.C"
                                 << file1Qml << 7 << positionAfterOneIndent;

    QTest::addRow("onCProperty2") << file1Qml << 28 << 37 << firstResult << outOfOne << "file1.C"
                                  << file1Qml << 7 << positionAfterOneIndent;

    QTest::addRow("onCProperty3") << file1Qml << 28 << 35 << firstResult << outOfOne << "file1.C"
                                  << file1Qml << 7 << positionAfterOneIndent;

    QTest::addRow("onCBinding") << file1Qml << 46 << 8 << firstResult << outOfOne << "file1.C"
                                << file1Qml << 7 << positionAfterOneIndent;

    QTest::addRow("onDefaultBinding")
            << file1Qml << 16 << positionAfterOneIndent << firstResult << outOfOne << "file1.C"
            << file1Qml << 7 << positionAfterOneIndent;

    QTest::addRow("onDefaultBindingId")
            << file1Qml << 16 << 28 << firstResult << outOfOne << "firstD" << file1Qml << 16 << 20;

    QTest::addRow("findIntProperty") << file1Qml << 9 << 18 << firstResult << outOfOne << u"int"_s
                                     << file1Qml << -1 << positionAfterOneIndent;

    QTest::addRow("colorBinding") << file1Qml << 39 << 8 << firstResult << outOfOne << u"QColor"_s
                                  << file1Qml << -1 << positionAfterOneIndent;

    // check what happens between items (it should not crash)

    QTest::addRow("onWhitespaceBeforeC") << file1Qml << 16 << 1 << firstResult << outOfOne
                                         << noResultExpected << noResultExpected << -1 << -1;

    QTest::addRow("onWhitespaceAfterC") << file1Qml << 17 << 23 << firstResult << outOfOne
                                        << noResultExpected << noResultExpected << -1 << -1;

    QTest::addRow("onWhitespaceBetweenCAndD") << file1Qml << 17 << 24 << firstResult << outOfOne
                                              << noResultExpected << noResultExpected << -1 << -1;

    QTest::addRow("ic") << file1Qml << 15 << 15 << firstResult << outOfOne << u"icid"_s << file1Qml
                        << -1 << 18;
    QTest::addRow("icBase") << file1Qml << 15 << 20 << firstResult << outOfOne << u"QQuickItem"_s
                            << u"TODO: file location for C++ defined types?"_s << -1 << -1;
    QTest::addRow("ic3") << file1Qml << 15 << 33 << firstResult << outOfOne << u"icid"_s << file1Qml
                         << -1 << 18;

    // TODO: type definition of function = type definition of return type?
    // if not, this might need fixing:
    // currently, asking the type definition of the "function" keyword returns
    // the type definitin of the return type (when available).
    QTest::addRow("function-keyword") << file1Qml << 33 << 5 << firstResult << outOfOne
                                      << u"file1.C"_s << file1Qml << 7 << positionAfterOneIndent;
    QTest::addRow("function-parameter-builtin")
            << file1Qml << 33 << 20 << firstResult << outOfOne << "a" << file1Qml << -1 << -1;
    QTest::addRow("function-parameter-item")
            << file1Qml << 33 << 36 << firstResult << outOfOne << "file1.C" << file1Qml << 7
            << positionAfterOneIndent;

    QTest::addRow("function-return") << file1Qml << 33 << 41 << firstResult << outOfOne << "file1.C"
                                     << file1Qml << 7 << positionAfterOneIndent;

    QTest::addRow("void-function") << file1Qml << 36 << 17 << firstResult << outOfOne << "void"
                                   << noResultExpected << -1 << -1;

    QTest::addRow("rectangle-property")
            << file1Qml << 44 << 31 << firstResult << outOfOne << "QQuickRectangle"
            << "TODO: c++ type location" << -1 << -1;

    QTest::addRow("functionParameterICUsage")
            << file1Qml << 34 << 16 << firstResult << outOfOne << "file1.C" << file1Qml << 7 << 18;

    QTest::addRow("ICBindingUsage")
            << file1Qml << 47 << 21 << firstResult << outOfOne << "file1.C" << file1Qml << 7 << 18;
    QTest::addRow("ICBindingUsage2")
            << file1Qml << 49 << 11 << firstResult << outOfOne << "file1.C" << file1Qml << 7 << 18;
    QTest::addRow("ICBindingUsage3")
            << file1Qml << 52 << 17 << firstResult << outOfOne << "file1.C" << file1Qml << 7 << 18;
}

void tst_qmlls_utils::findTypeDefinitionFromLocation()
{
    QFETCH(QString, filePath);
    QFETCH(int, line);
    QFETCH(int, character);
    QFETCH(int, resultIndex);
    QFETCH(int, expectedItemsCount);
    QFETCH(QString, expectedTypeName);
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

    QQmlJS::Dom::DomItem type = QQmlLSUtils::findTypeDefinitionOf(locations[resultIndex].domItem);

    // if expectedFilePath is empty, we probably just want to make sure that it does
    // not crash
    if (expectedFilePath == noResultExpected) {
        QCOMPARE(type.internalKind(), QQmlJS::Dom::DomType::Empty);
        return;
    }

    QVERIFY(type);

    QQmlJS::Dom::FileLocations::Tree typeLocationToTest = QQmlJS::Dom::FileLocations::treeOf(type);

    QEXPECT_FAIL("findIntProperty", "Builtins not supported yet", Abort);
    QEXPECT_FAIL("function-parameter-builtin", "Base types defined in C++ are not supported yet",
                 Abort);
    QEXPECT_FAIL("colorBinding", "Types from C++ bases not supported yet", Abort);
    QEXPECT_FAIL("rectangle-property", "Types from C++ bases not supported yet", Abort);
    QEXPECT_FAIL("icBase", "Base types defined in C++ are not supported yet", Abort);

    auto fileObject = type.containingFile().as<QQmlJS::Dom::QmlFile>();

    // print some debug message when failing, instead of using QVERIFY2
    // (printing the type every time takes a lot of time).
    if constexpr (enable_debug_output) {
        if (!fileObject)
            qDebug() << "This has no file: " << type.containingFile() << " for type "
                     << type.field(QQmlJS::Dom::Fields::type).get().component();
    }

    QVERIFY(fileObject);
    QCOMPARE(fileObject->canonicalFilePath(), expectedFilePath);

    QCOMPARE(typeLocationToTest->info().fullRegion.startLine, quint32(expectedLine));
    QCOMPARE(typeLocationToTest->info().fullRegion.startColumn, quint32(expectedCharacter));

    if (auto object = type.as<QQmlJS::Dom::QmlObject>()) {
        const std::vector<QString> except = {
            "functionParameterICUsage",
            "ICBindingUsage",
            "ICBindingUsage2",
            "ICBindingUsage3",
        };
        for (const QString &functionName : except) {
            QEXPECT_FAIL(
                    functionName.toStdString().c_str(),
                    "Types for JS-identifiers point to the type inside inline components instead "
                    "of the inline component itself",
                    Continue);
        }
        QCOMPARE(object->idStr(), expectedTypeName);
    } else if (auto exported = type.as<QQmlJS::Dom::Export>()) {
        QCOMPARE(exported->typeName, expectedTypeName);
    } else {
        if constexpr (enable_debug_output) {
            if (type.name() != expectedTypeName)
                qDebug() << " has not the unexpected name " << type.name()
                         << " instead of expected " << expectedTypeName << " in " << type;
        }
        QCOMPARE(type.name(), expectedTypeName);
    }
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

    auto type = QQmlLSUtils::findTypeDefinitionOf(locations.front().domItem);
    auto base = QQmlLSUtils::baseObject(type);
    QByteArray failOnInlineComponentsMessage =
            "The Dom cannot resolve inline components from the basetype yet.";

    QEXPECT_FAIL("inline-ic", failOnInlineComponentsMessage, Abort);
    QEXPECT_FAIL("inline-ic-from-id", failOnInlineComponentsMessage, Abort);
    QEXPECT_FAIL("inline-ic2", failOnInlineComponentsMessage, Abort);
    QEXPECT_FAIL("inline-ic2-from-id", failOnInlineComponentsMessage, Abort);

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

using QQmlJS::SourceLocation;

static QQmlLSUtilsLocation sourceLocationFrom(const QString fileName, const QString &code,
                                              quint32 startLine, quint32 startCharacter,
                                              quint32 length)
{
    quint32 offset = QQmlLSUtils::textOffsetFrom(code, startLine - 1, startCharacter - 1);

    QQmlLSUtilsLocation location{
        fileName, QQmlJS::SourceLocation{ offset, length, startLine, startCharacter }
    };
    return location;
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
        sourceLocationFrom(testFileName, testFileContent, 8, 13, strlen("sum")),
        sourceLocationFrom(testFileName, testFileContent, 10, 13, strlen("sum")),
        sourceLocationFrom(testFileName, testFileContent, 10, 19, strlen("sum")),
    };

    QList<QQmlLSUtilsLocation> iUsages{
        sourceLocationFrom(testFileName, testFileContent, 9, 17, strlen("i")),
        sourceLocationFrom(testFileName, testFileContent, 9, 24, strlen("i")),
        sourceLocationFrom(testFileName, testFileContent, 9, 32, strlen("i")),
        sourceLocationFrom(testFileName, testFileContent, 9, 36, strlen("i")),
        sourceLocationFrom(testFileName, testFileContent, 10, 25, strlen("i")),
    };

    QList<QQmlLSUtilsLocation> helloPropertyUsages{
        sourceLocationFrom(testFileName, testFileContent, 17, 5,
                           strlen("property int helloProperty: 0")),
        sourceLocationFrom(testFileName, testFileContent, 24, 13, strlen("helloProperty")),
        sourceLocationFrom(testFileName, testFileContent, 24, 29, strlen("helloProperty")),
        sourceLocationFrom(testFileName, testFileContent, 65, 60, strlen("helloProperty")),
    };

    QList<QQmlLSUtilsLocation> subItemHelloPropertyUsages{
        sourceLocationFrom(testFileName, testFileContent, 32, 20, strlen("helloProperty")),
        sourceLocationFrom(testFileName, testFileContent, 34, 9,
                           strlen("property string helloProperty")),
    };

    QList<QQmlLSUtilsLocation> ICHelloPropertyUsages{
        sourceLocationFrom(testFileName, testFileContent, 37, 9,
                           strlen("property var helloProperty")),
        sourceLocationFrom(testFileName, testFileContent, 39, 20, strlen("helloProperty")),
    };

    QList<QQmlLSUtilsLocation> p2Usages{
        sourceLocationFrom(testFileName, testFileContent, 18, 5, strlen("property int p2: 1")),
        sourceLocationFrom(testFileName, testFileContent, 24, 55, strlen("p2")),
        sourceLocationFrom(testFileName, testFileContent, 32, 36, strlen("p2")),
        sourceLocationFrom(testFileName, testFileContent, 39, 36, strlen("p2")),
        sourceLocationFrom(testFileName, testFileContent, 66, 31, strlen("p2")),
        sourceLocationFrom(testFileName, testFileContent, 67, 37, strlen("p2")),
        sourceLocationFrom(testFileName, testFileContent, 68, 43, strlen("p2")),
        sourceLocationFrom(testFileName, testFileContent, 69, 49, strlen("p2")),
    };

    QList<QQmlLSUtilsLocation> nestedUsages{
        sourceLocationFrom(testFileName, testFileContent, 62, 13, strlen("myNested")),
        sourceLocationFrom(testFileName, testFileContent, 65, 17, strlen("myNested")),
        sourceLocationFrom(testFileName, testFileContent, 66, 17, strlen("myNested")),
        sourceLocationFrom(testFileName, testFileContent, 67, 17, strlen("myNested")),
        sourceLocationFrom(testFileName, testFileContent, 68, 17, strlen("myNested")),
        sourceLocationFrom(testFileName, testFileContent, 69, 17, strlen("myNested")),
    };

    QList<QQmlLSUtilsLocation> nestedComponent3Usages{
        sourceLocationFrom(testFileName, testFileContent, 47, 9,
                           strlen("property NestedComponent3 inner")),
        sourceLocationFrom(testFileName, testFileContent, 65, 32, strlen("inner")),
        sourceLocationFrom(testFileName, testFileContent, 68, 32, strlen("inner")),
        sourceLocationFrom(testFileName, testFileContent, 69, 32, strlen("inner")),
    };

    QList<QQmlLSUtilsLocation> nestedComponent3P2Usages{
        sourceLocationFrom(testFileName, testFileContent, 68, 38, strlen("p2")),
        sourceLocationFrom(testFileName, testFileContent, 53, 9, strlen("property int p2")),
    };

    std::sort(sumUsages.begin(), sumUsages.end());
    std::sort(iUsages.begin(), iUsages.end());
    std::sort(subItemHelloPropertyUsages.begin(), subItemHelloPropertyUsages.end());
    std::sort(ICHelloPropertyUsages.begin(), ICHelloPropertyUsages.end());
    std::sort(p2Usages.begin(), p2Usages.end());
    std::sort(nestedUsages.begin(), nestedUsages.end());
    std::sort(nestedComponent3Usages.begin(), nestedComponent3Usages.end());
    std::sort(nestedComponent3P2Usages.begin(), nestedComponent3P2Usages.end());

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

    QTest::addRow("findPropertyFromUsageInFieldMemberExpression")
            << testFileName << 69 << 34 << nestedComponent3Usages;

    QTest::addRow("findFieldMemberExpressionUsageFromPropertyDefinition")
            << testFileName << 47 << 38 << nestedComponent3Usages;

    QTest::addRow("findProperty2FromUsageInFieldMemberExpression")
            << testFileName << 68 << 39 << nestedComponent3P2Usages;
}

void tst_qmlls_utils::findUsages()
{
    QFETCH(QString, filePath);
    QFETCH(int, line);
    QFETCH(int, character);
    QFETCH(QList<QQmlLSUtilsLocation>, expectedUsages);

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
                qDebug() << x.filename << "(" << x.location.startLine << ", "
                         << x.location.startColumn << "), " << x.location.offset << "+"
                         << x.location.length;
            }
            qDebug() << "But expected: \n";
            for (auto &x : expectedUsages) {
                qDebug() << x.filename << "(" << x.location.startLine << ", "
                         << x.location.startColumn << "), " << x.location.offset << "+"
                         << x.location.length;
            }
        }
    }
    QCOMPARE(usages, expectedUsages);
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
    QTest::addRow("propertyI") << JSDefinitionsQml << 14 << 14 << JSDefinitionsQml << 9
                               << positionAfterOneIndent << strlen("property int i");
    QTest::addRow("qualifiedPropertyI") << JSDefinitionsQml << 15 << 21 << JSDefinitionsQml << 9
                                        << positionAfterOneIndent << strlen("property int i");
    QTest::addRow("inlineComponentProperty") << JSDefinitionsQml << 62 << 21 << JSDefinitionsQml
                                             << 54 << 9 << strlen("property int data: 42");

    QTest::addRow("parameterA") << JSDefinitionsQml << 10 << 16 << noResultExpected << -1 << -1
                                << size_t{};
    QTest::addRow("parameterB") << JSDefinitionsQml << 10 << 28 << noResultExpected << -1 << -1
                                << size_t{};
    QTest::addRow("comment") << JSDefinitionsQml << 10 << 21 << noResultExpected << -1 << -1
                             << size_t{};

    QTest::addRow("scopedX") << JSDefinitionsQml << 22 << 18 << JSDefinitionsQml << 21 << 17
                             << strlen("scoped");
    QTest::addRow("scopedX2") << JSDefinitionsQml << 25 << 22 << JSDefinitionsQml << 21 << 17
                              << strlen("scoped");
    QTest::addRow("scopedX3") << JSDefinitionsQml << 28 << 14 << JSDefinitionsQml << 19 << 13
                              << strlen("scoped");

    QTest::addRow("normalI") << JSDefinitionsQml << 22 << 23 << JSDefinitionsQml << 9
                             << positionAfterOneIndent << strlen("property int i");
    QTest::addRow("scopedI") << JSDefinitionsQml << 25 << 27 << JSDefinitionsQml << 24 << 32
                             << strlen("i");

    QTest::addRow("propertyInBinding") << JSDefinitionsQml << 64 << 37 << JSDefinitionsQml << 9
                                       << positionAfterOneIndent << strlen("property int i");
    QTest::addRow("propertyInBinding2") << JSDefinitionsQml << 65 << 38 << JSDefinitionsQml << 9
                                        << positionAfterOneIndent << strlen("property int i");
    QTest::addRow("propertyInBinding3") << JSDefinitionsQml << 66 << 51 << JSDefinitionsQml << 9
                                        << positionAfterOneIndent << strlen("property int i");

    QTest::addRow("propertyFromDifferentFile")
            << JSDefinitionsQml << 72 << 20 << BaseTypeQml << 24 << positionAfterOneIndent
            << strlen("property int helloProperty: 123");

    QTest::addRow("id") << JSDefinitionsQml << 15 << 17 << JSDefinitionsQml << 6 << 1
                        << strlen("Item");
    QTest::addRow("onId") << JSDefinitionsQml << 32 << 16 << JSDefinitionsQml << 31
                          << positionAfterOneIndent << strlen("Rectangle");
    QTest::addRow("inlineComponentId")
            << JSDefinitionsQml << 56 << 35 << JSDefinitionsQml << 51 << 21 << strlen("Rectangle");
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

    auto type = QQmlLSUtils::findDefinitionOf(locations.front().domItem);

    // if expectedFilePath is empty, we probably just want to make sure that it does
    // not crash
    if (expectedFilePath == noResultExpected) {
        QVERIFY(!type);
        return;
    }

    QVERIFY(type);

    QCOMPARE(type->filename, expectedFilePath);

    QCOMPARE(type->location.startLine, quint32(expectedLine));
    QCOMPARE(type->location.startColumn, quint32(expectedCharacter));
    QCOMPARE(type->location.length, quint32(expectedLength));
}

QTEST_MAIN(tst_qmlls_utils)
