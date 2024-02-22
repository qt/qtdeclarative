// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQmlDom/private/qqmldomtop_p.h>
#include <QtQmlDom/private/qqmldomitem_p.h>

#include <QtTest/QtTest>
#include <QtCore/QLibraryInfo>

class tst_qmldomconstruction : public QObject
{
    Q_OBJECT

private slots:
    void domConstructionTime_data();
    void domConstructionTime();
};

void tst_qmldomconstruction::domConstructionTime_data()
{
    using namespace QQmlJS::Dom;
    using namespace Qt::StringLiterals;

    const auto baseDir = QLatin1String(SRCDIR) + QLatin1String("/data");
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<DomCreationOptions>("withScope");

    DomCreationOptions withScope = DomCreationOption::WithSemanticAnalysis;
    DomCreationOptions noScope = DomCreationOption::None;
    DomCreationOptions withScopeAndScriptExpressions;
    withScopeAndScriptExpressions.setFlag(DomCreationOption::WithSemanticAnalysis);
    withScopeAndScriptExpressions.setFlag(DomCreationOption::WithScriptExpressions);

    QTest::addRow("tiger.qml") << baseDir + u"/longQmlFile.qml"_s << noScope;
    QTest::addRow("tiger.qml-with-scope") << baseDir + u"/longQmlFile.qml"_s << withScope;
    QTest::addRow("tiger.qml-with-scope-and-scriptexpressions")
            << baseDir + u"/longQmlFile.qml"_s << withScopeAndScriptExpressions;

    QTest::addRow("deeplyNested.qml") << baseDir + u"/deeplyNested.qml"_s << noScope;
    QTest::addRow("deeplyNested.qml-with-scope") << baseDir + u"/deeplyNested.qml"_s << withScope;
    QTest::addRow("deeplyNested.qml-with-scope-and-scriptexpressions")
            << baseDir + u"/deeplyNested.qml"_s << withScopeAndScriptExpressions;
}

void tst_qmldomconstruction::domConstructionTime()
{
    using namespace QQmlJS::Dom;
    QFETCH(QString, fileName);
    QFETCH(DomCreationOptions, withScope);

    const QStringList importPaths = {
        QLibraryInfo::path(QLibraryInfo::QmlImportsPath),
    };

    DomItem tFile;
    QBENCHMARK {
        DomItem env = DomEnvironment::create(
                importPaths,
                QQmlJS::Dom::DomEnvironment::Option::SingleThreaded
                        | QQmlJS::Dom::DomEnvironment::Option::NoDependencies);

        env.loadFile(
                FileToLoad::fromFileSystem(env.ownerAs<DomEnvironment>(), fileName, withScope),
                [&tFile](Path, const DomItem &, const DomItem &newIt) { tFile = newIt.fileObject(); },
                LoadOption::DefaultLoad);
        env.loadPendingDependencies();
    }
}

QTEST_MAIN(tst_qmldomconstruction)
#include "tst_qmldomconstruction.moc"
