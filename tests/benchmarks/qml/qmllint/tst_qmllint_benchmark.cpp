// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>
#include <QtCore/qobject.h>
#include <QtCore/QLibraryInfo>
#include <QtCore/qdirlisting.h>
#include <private/qqmljslinter_p.h>

using namespace Qt::StringLiterals;

class tst_qmllint_benchmark : public QObject
{
    Q_OBJECT

public:
    // See also printEnvironment.sh for the environment variables.
    tst_qmllint_benchmark(QObject *parent = nullptr)
        : QObject(parent),
          m_importPath(qEnvironmentVariable("TST_QMLLINT_BENCHMARK_IMPORT_PATH")
                               .split(QDir::listSeparator())),
          m_qdsProjectsToLint(qEnvironmentVariable("TST_QMLLINT_BENCHMARK_PROJECTS")
                                      .split(QDir::listSeparator())),
          m_useHeuristics(qEnvironmentVariableIsSet("TST_QMLLINT_BENCHMARK_IMPORT_PATH_HEURISTICS"))
    {
    }
    enum PluginSelection { NoPlugins, AllPlugins, OnlyQdsLintPlugin, CompilerWarnings };
    void runOnFile(const QString &fileName, PluginSelection allowedPlugins);

private slots:

    void noPlugins_data();
    void noPlugins();

    void allPlugins_data();
    void allPlugins();

    void onlyQdsLintPlugin_data();
    void onlyQdsLintPlugin();

    void compilerWarnings_data();
    void compilerWarnings();

private:
    const QString m_baseDir = QLatin1String(SRCDIR) + QLatin1String("/data/");
    static constexpr std::array m_files = {
        "propertyStressTestInts.ui.qml"_L1,
        "propertyStressTestItems.ui.qml"_L1,
        "longQmlFile.ui.qml"_L1,
        "deeplyNested.ui.qml"_L1,
    };

    // import path to current qt version, and to QDS specific QML modules
    const QStringList m_importPath;

    // recommendations:
    //  * industrial-automation-hid/Development/responsive-scada
    //  * qtdesign-studio/examples/MaterialBundleDemo
    //  * qtdesign-studio/examples/EosADAS
    //  * qtdesign-studio/examples/OutrunHVAC
    //  * qtdesign-studio/examples/DesignEffectsDemo
    const QStringList m_qdsProjectsToLint;
    bool m_useHeuristics = false;
};

static bool hasQmlProjectFile(const QString &dir)
{
    const auto listing = QDirListing(dir, QStringList{ "*.qmlproject"_L1 });
    return listing.cbegin() != listing.cend();
}

static QStringList extraImportsForQdsProjects(const QString &fileName)
{
    QStringList imports;

    // heuristics to find the extra import paths, shipped with the QDS demos/examples
    int maxDepth = 5;
    QString projectPath(QDir::cleanPath(fileName + "/.."));
    while (projectPath.contains(u'/') && !hasQmlProjectFile(projectPath)) {
        projectPath.chop(projectPath.size() - projectPath.lastIndexOf(u'/'));
        if (--maxDepth == 0)
            return imports;
    }
    if (projectPath.isEmpty())
        return imports;

    // over-approximation where the import paths are set, to avoid parsing the content from
    // .qmlproject to find which import paths are set.
    imports.append(projectPath);
    imports.append(QDir::cleanPath(projectPath + "/imports"));
    imports.append(QDir::cleanPath(projectPath + "/asset_imports"));
    imports.append(QDir::cleanPath(projectPath + "/backend"));
    imports.append(QDir::cleanPath(projectPath + "/content"));

    return imports;
}

void tst_qmllint_benchmark::runOnFile(const QString &fileName, PluginSelection allowedPlugins)
{
    QStringList imports = m_importPath;
    imports.append(QLibraryInfo::paths(QLibraryInfo::QmlImportsPath));

    if (m_useHeuristics && allowedPlugins == OnlyQdsLintPlugin)
        imports += extraImportsForQdsProjects(fileName);

    QQmlJSLinter linter(imports);

    QList<QQmlJS::LoggerCategory> categories = QQmlJSLogger::builtinCategories();
    switch (allowedPlugins) {
    case CompilerWarnings:
        for (auto &category : categories) {
            if (category.id() == qmlCompiler)
                category.setSeverity(QQmlSA::WarningSeverity::Warning);
        }
        Q_FALLTHROUGH(); // disable the plugins, concentrate on the compiler warnings
    case NoPlugins:
        for (QQmlJSLinter::Plugin &plugin : linter.plugins())
            plugin.setEnabled(false);
        break;
    case AllPlugins:
        for (QQmlJSLinter::Plugin &plugin : linter.plugins()) {
            for (const QQmlJS::LoggerCategory &category : plugin.categories()) {
                categories.append(category);
            }
        }
        break;
    case OnlyQdsLintPlugin:
        for (QQmlJSLinter::Plugin &plugin : linter.plugins()) {
            if (plugin.name() != "QtDesignStudio") {
                plugin.setEnabled(false);
                continue;
            }
            for (const QQmlJS::LoggerCategory &category : plugin.categories())
                categories.append(category);
        }
        break;
    }

    const QString content = [&fileName, this]() {
        QFile file(fileName.startsWith(u'/') ? fileName : m_baseDir + fileName);
        [&file]() { QVERIFY(file.open(QFile::ReadOnly | QFile::Text)); }();
        return file.readAll();
    }();

    std::vector<QQmlJSLinter::Result> results;
    QQmlJSLinter::LintOptions options;
    options.setFlag(QQmlJSLinter::Silent);

    QVERIFY(linter.prepareFileForBatchLinting(fileName, &content, options, imports, { }, { },
                                              categories));
    QBENCHMARK {
        results.push_back(linter.lintFileInBatch(fileName));
    }
    // make sure that all imports are available, otherwise we end up benchmarking less relevant
    // parts of the execution.
    if (results.size() > 0 && results.front().logger) {
        results.front().logger->iterateAllMessages([](const Message &message) {
            QVERIFY(!message.message.contains("Failed to import"_L1));
        });
    }
}

void tst_qmllint_benchmark::allPlugins_data()
{
    QTest::addColumn<QLatin1String>("fileName");

    for (const auto &file : m_files)
        QTest::addRow("%s", file.data()) << file;
}

void tst_qmllint_benchmark::allPlugins()
{
    QFETCH(QLatin1String, fileName);

    runOnFile(fileName, AllPlugins);
}

void tst_qmllint_benchmark::noPlugins_data()
{
    QTest::addColumn<QLatin1String>("fileName");

    for (const auto &file : m_files)
        QTest::addRow("%s", file.data()) << file;
}

void tst_qmllint_benchmark::noPlugins()
{
    QFETCH(QLatin1String, fileName);

    runOnFile(fileName, NoPlugins);
}

void tst_qmllint_benchmark::onlyQdsLintPlugin_data()
{
    QTest::addColumn<QString>("fileName");

    for (const auto &dir : m_qdsProjectsToLint) {
        for (const auto &dirEntry :
             QDirListing(dir, QStringList{ "*.ui.qml"_L1 }, QDirListing::IteratorFlag::Recursive)) {
            const QString path = dirEntry.absoluteFilePath();
            QTest::addRow("%s", path.toUtf8().constData()) << path;
        }
    }

    for (const auto &file : m_files)
        QTest::addRow("%s", file.data()) << file.toString();
}

void tst_qmllint_benchmark::onlyQdsLintPlugin()
{
    QFETCH(QString, fileName);

    runOnFile(fileName, OnlyQdsLintPlugin);
}

void tst_qmllint_benchmark::compilerWarnings_data()
{
    QTest::addColumn<QLatin1String>("fileName");

    for (const auto &file : m_files)
        QTest::addRow("%s", file.data()) << file;
}

void tst_qmllint_benchmark::compilerWarnings()
{
    QFETCH(QLatin1String, fileName);

    runOnFile(fileName, CompilerWarnings);
}

QTEST_MAIN(tst_qmllint_benchmark)
#include "tst_qmllint_benchmark.moc"
