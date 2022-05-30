// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest>
#include <QtQml>
#include <QtCore/private/qhooks_p.h>
#include <QtCore/qpair.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qset.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmljsast_p.h>
#include <QtQmlCompiler/private/qqmljslogger_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmlmetatype_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>
#include <QtQmlCompiler/private/qqmljslinter_p.h>

using namespace QQuickVisualTestUtils;
using namespace QQuickControlsTestUtils;
using namespace Qt::StringLiterals;

class tst_Sanity : public QObject
{
    Q_OBJECT
public:
    tst_Sanity();

private slots:
    void initTestCase();

    void jsFiles();
    void qmllint();
    void qmllint_data();

private:
    QMap<QString, QString> sourceQmlFiles;
    QMap<QString, QString> installedQmlFiles;
    QStringList m_importPaths;

    QQmlJSLinter m_linter;
    QList<QQmlJSLogger::Category> m_categories;
};

tst_Sanity::tst_Sanity()
    : m_importPaths({ QLibraryInfo::path(QLibraryInfo::QmlImportsPath) }),
      m_linter(m_importPaths),
      m_categories(QQmlJSLogger::defaultCategories())
{
    // We do not care about any warnings that aren't explicitly created by controls-sanity.
    // Mainly because a lot of false positives are generated because we are linting files from
    // different modules directly without their generated qmldirs.

    m_linter.setPluginsEnabled(false);

    for (auto &category : m_categories) {
        if (category == qmlDeferredPropertyId || category == qmlControlsSanity
            || category == qmlAttachedPropertyReuse) {
            category.setLevel(u"warning"_s);
        } else {
            category.setLevel(u"disable"_s);
        }
    }
}

void tst_Sanity::initTestCase()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(QString("import QtQuick.Templates 2.%1; Control { }").arg(15).toUtf8(), QUrl());

    const QStringList qmlTypeNames = QQmlMetaType::qmlTypeNames();

    // Collect the files from each style in the source tree.
    QDirIterator it(QQC2_IMPORT_PATH, QStringList() << "*.qml" << "*.js", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QFileInfo info = it.fileInfo();
        if (qmlTypeNames.contains(QStringLiteral("QtQuick.Templates/") + info.baseName()))
            sourceQmlFiles.insert(info.dir().dirName() + "/" + info.fileName(), info.filePath());
    }

    // Then, collect the files from each installed style directory.
    const QVector<QPair<QString, QString>> styleRelativePaths = {
        { "basic", "QtQuick/Controls/Basic" },
        { "fusion", "QtQuick/Controls/Fusion" },
        { "material", "QtQuick/Controls/Material" },
        { "universal", "QtQuick/Controls/Universal" },
        // TODO: add native styles: QTBUG-87108
        { "ios", "QtQuick/Controls/iOS" }
    };
    for (const auto &stylePathPair : styleRelativePaths) {
        forEachControl(&engine, QQC2_IMPORT_PATH, stylePathPair.first, stylePathPair.second, QStringList(),
                [&](const QString &relativePath, const QUrl &absoluteUrl) {
             installedQmlFiles.insert(relativePath, absoluteUrl.toLocalFile());
        });
    }
}

void tst_Sanity::jsFiles()
{
    QMap<QString, QString>::const_iterator it;
    for (it = sourceQmlFiles.constBegin(); it != sourceQmlFiles.constEnd(); ++it) {
        if (QFileInfo(it.value()).suffix() == QStringLiteral("js"))
            QFAIL(qPrintable(it.value() +  ": JS files are not allowed"));
    }
}

void tst_Sanity::qmllint()
{
    QFETCH(QString, control);
    QFETCH(QString, filePath);

    QJsonArray output;
    bool success =
            m_linter.lintFile(filePath, nullptr, true, &output, m_importPaths, {}, {}, m_categories)
            == QQmlJSLinter::LintSuccess;

    QVERIFY2(success, qPrintable(QJsonDocument(output).toJson(QJsonDocument::Compact)));
}

void tst_Sanity::qmllint_data()
{
    QTest::addColumn<QString>("control");
    QTest::addColumn<QString>("filePath");

    QMap<QString, QString>::const_iterator it;
    for (it = sourceQmlFiles.constBegin(); it != sourceQmlFiles.constEnd(); ++it)
        QTest::newRow(qPrintable(it.key())) << it.key() << it.value();
}

QTEST_MAIN(tst_Sanity)

#include "tst_sanity.moc"
