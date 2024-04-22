// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QtTest>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QString>
#include <QTemporaryDir>
#include <QtTest/private/qemulationdetector_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQmlDom/private/qqmldomitem_p.h>
#include <QtQmlDom/private/qqmldomlinewriter_p.h>
#include <QtQmlDom/private/qqmldomoutwriter_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>

using namespace QQmlJS::Dom;

class E2ETestQmlformat : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase();

#if !defined(QTEST_CROSS_COMPILED) // sources not available when cross compiled
    void testExample();
    void testExample_data();
    void normalizeExample();
    void normalizeExample_data();
#endif

private:
    //TODO(QTBUG-117849) refactor this helper function
    QString formatInMemory(const QString &fileToFormat, bool *didSucceed = nullptr,
                           LineWriterOptions options = LineWriterOptions(),
                           WriteOutChecks extraChecks = WriteOutCheck::ReparseCompare,
                           WriteOutChecks largeChecks = WriteOutCheck::None);

    QString m_qmlformatPath;
    QStringList m_excludedDirs;
    QStringList m_invalidFiles;
    QStringList m_ignoreFiles;

    QStringList findFiles(const QDir &);
    bool isInvalidFile(const QFileInfo &fileName) const;
    bool isIgnoredFile(const QFileInfo &fileName) const;
};

void E2ETestQmlformat::initTestCase()
{
    // QQmlDataTest::initTestCase();
    m_qmlformatPath = QLibraryInfo::path(QLibraryInfo::BinariesPath) + QLatin1String("/qmlformat");
#ifdef Q_OS_WIN
    m_qmlformatPath += QLatin1String(".exe");
#endif
    if (!QFileInfo(m_qmlformatPath).exists()) {
        QString message = QStringLiteral("qmlformat executable not found (looked for %0)").arg(m_qmlformatPath);
        QFAIL(qPrintable(message));
    }

    // Add directories you want excluded here

    // These snippets are not expected to run on their own.
    m_excludedDirs << "doc/src/snippets/qml/visualdatamodel_rootindex";
    m_excludedDirs << "doc/src/snippets/qml/qtbinding";
    m_excludedDirs << "doc/src/snippets/qml/imports";
    m_excludedDirs << "doc/src/snippets/qtquick1/visualdatamodel_rootindex";
    m_excludedDirs << "doc/src/snippets/qtquick1/qtbinding";
    m_excludedDirs << "doc/src/snippets/qtquick1/imports";
    m_excludedDirs << "tests/manual/v4";
    m_excludedDirs << "tests/manual/qmllsformatter";
    m_excludedDirs << "tests/auto/qml/ecmascripttests";
    m_excludedDirs << "tests/auto/qml/qmllint";

    // Add invalid files (i.e. files with syntax errors)
    m_invalidFiles << "tests/auto/quick/qquickloader/data/InvalidSourceComponent.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/signal.2.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/signal.3.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/signal.5.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/property.4.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/empty.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/missingObject.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/insertedSemicolon.1.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/nonexistantProperty.5.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/invalidRoot.1.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/invalidQmlEnumValue.1.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/invalidQmlEnumValue.2.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/invalidQmlEnumValue.3.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/invalidID.4.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/questionDotEOF.qml";
    m_invalidFiles << "tests/auto/qml/qquickfolderlistmodel/data/dummy.qml";
    m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/stringParsing_error.1.qml";
    m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/stringParsing_error.2.qml";
    m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/stringParsing_error.3.qml";
    m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/stringParsing_error.4.qml";
    m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/stringParsing_error.5.qml";
    m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/stringParsing_error.6.qml";
    m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/numberParsing_error.1.qml";
    m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/numberParsing_error.2.qml";
    m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/incrDecrSemicolon_error1.qml";
    m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/incrDecrSemicolon_error1.qml";
    m_invalidFiles << "tests/auto/qml/debugger/qqmlpreview/data/broken.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/fuzzed.2.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/fuzzed.3.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/requiredProperties.2.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/nullishCoalescing_LHS_And.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/nullishCoalescing_LHS_And.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/nullishCoalescing_LHS_Or.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/nullishCoalescing_RHS_And.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/nullishCoalescing_RHS_Or.qml";
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/typeAnnotations.2.qml";
    m_invalidFiles << "tests/auto/qml/qqmlparser/data/disallowedtypeannotations/qmlnestedfunction.qml";
    m_invalidFiles << "tests/auto/qmlls/utils/data/emptyFile.qml";
    m_invalidFiles << "tests/auto/qmlls/utils/data/completions/missingRHS.qml";
    m_invalidFiles << "tests/auto/qmlls/utils/data/completions/missingRHS.parserfail.qml";
    m_invalidFiles << "tests/auto/qmlls/utils/data/completions/attachedPropertyMissingRHS.qml";
    m_invalidFiles << "tests/auto/qmlls/utils/data/completions/groupedPropertyMissingRHS.qml";
    m_invalidFiles << "tests/auto/qmlls/utils/data/completions/afterDots.qml";
    m_invalidFiles << "tests/auto/qmlls/modules/data/completions/bindingAfterDot.qml";
    m_invalidFiles << "tests/auto/qmlls/modules/data/completions/defaultBindingAfterDot.qml";
    m_invalidFiles << "tests/auto/qmlls/utils/data/qualifiedModule.qml";

    // Files that get changed:
    // rewrite of import "bla/bla/.." to import "bla"
    m_invalidFiles << "tests/auto/qml/qqmlcomponent/data/componentUrlCanonicalization.4.qml";
    // block -> object in internal update
    m_invalidFiles << "tests/auto/qml/qqmlpromise/data/promise-executor-throw-exception.qml";
    // removal of unsupported indexing of Object declaration
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/hangOnWarning.qml";
    // removal of duplicated id
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/component.3.qml";
    // Optional chains are not permitted on the left-hand-side in assignments
    m_invalidFiles << "tests/auto/qml/qqmllanguage/data/optionalChaining.LHS.qml";
    // object literal with = assignements
    m_invalidFiles << "tests/auto/quickcontrols/controls/data/tst_scrollbar.qml";

    // These files rely on exact formatting
    m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/incrDecrSemicolon1.qml";
    m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/incrDecrSemicolon_error1.qml";
    m_invalidFiles << "tests/auto/qml/qqmlecmascript/data/incrDecrSemicolon2.qml";

    // These files are too big
    m_ignoreFiles << "tests/benchmarks/qml/qmldom/data/longQmlFile.qml";
    m_ignoreFiles << "tests/benchmarks/qml/qmldom/data/deeplyNested.qml";
}

QStringList E2ETestQmlformat::findFiles(const QDir &d)
{
    for (int ii = 0; ii < m_excludedDirs.size(); ++ii) {
        QString s = m_excludedDirs.at(ii);
        if (d.absolutePath().endsWith(s))
            return QStringList();
    }

    QStringList rv;

    const QStringList files = d.entryList(QStringList() << QLatin1String("*.qml"),
                                          QDir::Files);
    for (const QString &file: files) {
        QString absoluteFilePath = d.absoluteFilePath(file);
        if (!isIgnoredFile(QFileInfo(absoluteFilePath)))
            rv << absoluteFilePath;
    }

    const QStringList dirs = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot |
                                         QDir::NoSymLinks);
    for (const QString &dir: dirs) {
        QDir sub = d;
        sub.cd(dir);
        rv << findFiles(sub);
    }

    return rv;
}

bool E2ETestQmlformat::isInvalidFile(const QFileInfo &fileName) const
{
    for (const QString &invalidFile : m_invalidFiles) {
        if (fileName.absoluteFilePath().endsWith(invalidFile))
            return true;
    }
    return false;
}

bool E2ETestQmlformat::isIgnoredFile(const QFileInfo &fileName) const
{
    for (const QString &file : m_ignoreFiles) {
        if (fileName.absoluteFilePath().endsWith(file))
            return true;
    }
    return false;
}

#if !defined(QTEST_CROSS_COMPILED) // sources not available when cross compiled
void E2ETestQmlformat::testExample_data()
{
    if (QTestPrivate::isRunningArmOnX86())
        QSKIP("Crashes in QEMU. (timeout)");
    QTest::addColumn<QString>("file");

    QString examples = QLatin1String(SRCDIR) + "/../../../../../examples/";
    QString tests = QLatin1String(SRCDIR) + "/../../../../../tests/";

    QStringList files;
    files << findFiles(QDir(examples));
    files << findFiles(QDir(tests));

    for (const QString &file : files)
        QTest::newRow(qPrintable(file)) << file;
}

void E2ETestQmlformat::normalizeExample_data()
{
    if (QTestPrivate::isRunningArmOnX86())
        QSKIP("Crashes in QEMU. (timeout)");
    QTest::addColumn<QString>("file");

    QString examples = QLatin1String(SRCDIR) + "/../../../../../examples/";
    QString tests = QLatin1String(SRCDIR) + "/../../../../../tests/";

    // normalizeExample is similar to testExample, so we test it only on nExamples + nTests
    // files to avoid making too many
    QStringList files;
    const int nExamples = 10;
    int i = 0;
    for (const auto &f : findFiles(QDir(examples))) {
        files << f;
        if (++i == nExamples)
            break;
    }
    const int nTests = 10;
    i = 0;
    for (const auto &f : findFiles(QDir(tests))) {
        files << f;
        if (++i == nTests)
            break;
    }

    for (const QString &file : files)
        QTest::newRow(qPrintable(file)) << file;
}
#endif

#if !defined(QTEST_CROSS_COMPILED) // sources not available when cross compiled
void E2ETestQmlformat::testExample()
{
    QFETCH(QString, file);
    const bool isInvalid = isInvalidFile(QFileInfo(file));
    bool wasSuccessful;
    LineWriterOptions opts;
    opts.attributesSequence = LineWriterOptions::AttributesSequence::Preserve;
    QString output = formatInMemory(file, &wasSuccessful, opts);

    if (!isInvalid)
        QVERIFY(wasSuccessful && !output.isEmpty());
}

void E2ETestQmlformat::normalizeExample()
{
    QFETCH(QString, file);
    const bool isInvalid = isInvalidFile(QFileInfo(file));
    bool wasSuccessful;
    LineWriterOptions opts;
    opts.attributesSequence = LineWriterOptions::AttributesSequence::Normalize;
    QString output = formatInMemory(file, &wasSuccessful, opts);

    if (!isInvalid)
        QVERIFY(wasSuccessful && !output.isEmpty());
}
#endif

QString E2ETestQmlformat::formatInMemory(const QString &fileToFormat, bool *didSucceed,
                                         LineWriterOptions options, WriteOutChecks extraChecks,
                                         WriteOutChecks largeChecks)
{
    auto env = DomEnvironment::create(
            QStringList(), // as we load no dependencies we do not need any paths
            QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                    | QQmlJS::Dom::DomEnvironment::Option::NoDependencies);
    DomItem tFile;
    env->loadFile(FileToLoad::fromFileSystem(env, fileToFormat),
                  [&tFile](Path, const DomItem &, const DomItem &newIt) { tFile = newIt; });
    env->loadPendingDependencies();
    MutableDomItem myFile = tFile.field(Fields::currentItem);

    bool writtenOut;
    QString resultStr;
    if (myFile.field(Fields::isValid).value().toBool()) {
        WriteOutChecks checks = extraChecks;
        const qsizetype largeFileSize = 32000;
        if (tFile.field(Fields::code).value().toString().size() > largeFileSize)
            checks = largeChecks;

        QTextStream res(&resultStr);
        LineWriter lw([&res](QStringView s) { res << s; }, QLatin1String("*testStream*"), options);
        OutWriter ow(lw);
        ow.indentNextlines = true;
        DomItem qmlFile = tFile.field(Fields::currentItem);
        writtenOut = qmlFile.writeOutForFile(ow, checks);
        lw.eof();
        res.flush();
    }
    if (didSucceed)
        *didSucceed = writtenOut;
    return resultStr;
}

QTEST_MAIN(E2ETestQmlformat)
#include "e2e_qmlformat.moc"
