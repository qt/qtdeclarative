// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <QObject>
#include <QQmlEngine>
#include <QQmlComponent>
#include <private/qqmljsdiagnosticmessage_p.h>
#include <private/qqmldirparser_p.h>
#include <QDebug>
#include <QtQuickTestUtils/private/qmlutils_p.h>

#include <algorithm>

// Test the parsing of qmldir files

class tst_qqmldirparser : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qqmldirparser();

private slots:
    void parse_data();
    void parse();
};

tst_qqmldirparser::tst_qqmldirparser()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

namespace {

    QStringList toStringList(const QList<QQmlJS::DiagnosticMessage> &errors)
    {
        QStringList rv;

        for (const QQmlJS::DiagnosticMessage &e : errors)  {
            QString errorString = QLatin1String("qmldir");
            if (e.loc.startLine > 0) {
                errorString += QLatin1Char(':') + QString::number(e.loc.startLine);
                if (e.loc.startColumn > 0)
                    errorString += QLatin1Char(':') + QString::number(e.loc.startColumn);
            }

            errorString += QLatin1String(": ") + e.message;
            rv.append(errorString);
        }

        return rv;
    }

    QString toString(const QQmlDirParser::Plugin &p)
    {
        return p.name + QLatin1Char('|') + p.path;
    }

    QStringList toStringList(const QList<QQmlDirParser::Plugin> &plugins)
    {
        QStringList rv;

        foreach (const QQmlDirParser::Plugin &p, plugins)
            rv.append(toString(p));

        return rv;
    }

    QString toString(const QQmlDirParser::Component &c)
    {
        return c.typeName + QLatin1Char('|') + c.fileName + QLatin1Char('|')
            + QString::number(c.version.majorVersion()) + QLatin1Char('|')
            + QString::number(c.version.minorVersion())
            + QLatin1Char('|') + (c.internal ? "true" : "false");
    }

    QString toString(const QQmlDirParser::Import &i)
    {
        return i.module + QLatin1String("||")
            + QString::number(i.version.majorVersion()) + QLatin1Char('|')
            + QString::number(i.version.minorVersion())
            + QLatin1String("|true");
    }

    QStringList toStringList(const QQmlDirComponents &components)
    {
        QStringList rv;

        foreach (const QQmlDirParser::Component &c, components.values())
            rv.append(toString(c));

        std::sort(rv.begin(), rv.end());
        return rv;
    }

    QStringList toStringList(const QQmlDirImports &components)
    {
        QStringList rv;

        foreach (const QQmlDirParser::Import &c, components)
            rv.append(toString(c));

        std::sort(rv.begin(), rv.end());
        return rv;
    }

    QString toString(const QQmlDirParser::Script &s)
    {
        return s.nameSpace + QLatin1Char('|') + s.fileName + QLatin1Char('|')
            + QString::number(s.version.majorVersion()) + '|'
            + QString::number(s.version.minorVersion());
    }

    QStringList toStringList(const QList<QQmlDirParser::Script> &scripts)
    {
        QStringList rv;

        foreach (const QQmlDirParser::Script &s, scripts)
            rv.append(toString(s));

        return rv;
    }
}

void tst_qqmldirparser::parse_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<QString>("preferredPath");
    QTest::addColumn<QStringList>("errors");
    QTest::addColumn<QStringList>("plugins");
    QTest::addColumn<QStringList>("classnames");
    QTest::addColumn<QStringList>("components");
    QTest::addColumn<QStringList>("scripts");
    QTest::addColumn<QStringList>("dependencies");
    QTest::addColumn<bool>("designerSupported");

    QTest::newRow("empty")
        << "empty/qmldir"
        << QString()
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << false;

    QTest::newRow("no-content")
        << "no-content/qmldir"
        << QString()
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << false;

    QTest::newRow("one-section")
        << "one-section/qmldir"
        << QString()
        << (QStringList() << "qmldir:1: a component declaration requires two or three arguments, but 1 were provided")
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << false;

    QTest::newRow("four-sections")
        << "four-sections/qmldir"
        << QString()
        << (QStringList() << "qmldir:1: a component declaration requires two or three arguments, but 4 were provided")
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << false;

    QTest::newRow("incomplete-module")
        << "incomplete-module/qmldir"
        << QString()
        << (QStringList() << "qmldir:1: module identifier directive requires one argument, but 0 were provided")
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << false;

    QTest::newRow("excessive-module")
        << "excessive-module/qmldir"
        << QString()
        << (QStringList() << "qmldir:1: module identifier directive requires one argument, but 2 were provided")
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << false;

    QTest::newRow("repeated-module")
        << "repeated-module/qmldir"
        << QString()
        << (QStringList() << "qmldir:2: only one module identifier directive may be defined in a qmldir file")
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << false;

    QTest::newRow("non-first-module")
        << "non-first-module/qmldir"
        << QString()
        << (QStringList() << "qmldir:2: module identifier directive must be the first directive in a qmldir file")
        << (QStringList() << "foo|")
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << false;

    QTest::newRow("incomplete-plugin")
        << "incomplete-plugin/qmldir"
        << QString()
        << (QStringList() << "qmldir:1: plugin directive requires one or two arguments, but 0 were provided")
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << false;

    QTest::newRow("excessive-plugin")
        << "excessive-plugin/qmldir"
        << QString()
        << (QStringList() << "qmldir:1: plugin directive requires one or two arguments, but 3 were provided")
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << false;

    QTest::newRow("name-plugin")
        << "name-plugin/qmldir"
        << QString()
        << QStringList()
        << (QStringList() << "foo|")
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << false;

    QTest::newRow("name-path-plugin")
        << "name-path-plugin/qmldir"
        << QString()
        << QStringList()
        << (QStringList() << "foo|bar")
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << false;

    QTest::newRow("unversioned-component")
        << "unversioned-component/qmldir"
        << QString()
        << QStringList()
        << QStringList()
        << QStringList()
        << (QStringList() << "foo|bar|255|255|false")
        << QStringList()
        << QStringList()
        << false;

    QTest::newRow("invalid-versioned-component")
        << "invalid-versioned-component/qmldir"
        << QString()
        << (QStringList() << "qmldir:1: invalid version 100, expected <major>.<minor>")
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << false;

    QTest::newRow("versioned-component")
        << "versioned-component/qmldir"
        << QString()
        << QStringList()
        << QStringList()
        << QStringList()
        << (QStringList() << "foo|bar|33|66|false")
        << QStringList()
        << QStringList()
        << false;

    QTest::newRow("invalid-versioned-script")
        << "invalid-versioned-script/qmldir"
        << QString()
        << (QStringList() << "qmldir:1: invalid version 100, expected <major>.<minor>")
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << false;

    QTest::newRow("versioned-script")
        << "versioned-script/qmldir"
        << QString()
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << (QStringList() << "foo|bar.js|33|66")
        << QStringList()
        << false;

    QTest::newRow("multiple")
        << "multiple/qmldir"
        << QString()
        << QStringList()
        << (QStringList() << "PluginA|plugina.so")
        << QStringList()
        << (QStringList() << "ComponentA|componenta-1_0.qml|1|0|false"
                          << "ComponentA|componenta-1_5.qml|1|5|false"
                          << "ComponentB|componentb-1_5.qml|1|5|false")
        << (QStringList() << "ScriptA|scripta-1_0.js|1|0")
        << QStringList()
        << false;

    QTest::newRow("designersupported-yes")
        << "designersupported-yes/qmldir"
        << QString()
        << QStringList()
        << (QStringList() << "foo|")
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << true;

    QTest::newRow("designersupported-no")
        << "designersupported-no/qmldir"
        << QString()
        << QStringList()
        << (QStringList() << "foo|")
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << false;

    QTest::newRow("invalid-versioned-dependency")
        << "invalid-versioned-dependency/qmldir"
        << QString()
        << (QStringList() << "qmldir:1: invalid version 100, expected <major>.<minor>")
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList()
        << false;

    QTest::newRow("dependency")
            << "dependency/qmldir"
            << QString()
            << QStringList()
            << (QStringList() << "foo|")
            << QStringList()
            << QStringList()
            << QStringList()
            << (QStringList() << "bar||1|0|true")
            << false;

    QTest::newRow("classname")
            << "classname/qmldir"
            << QString()
            << QStringList()
            << (QStringList() << "qtquick2plugin|")
            << (QStringList() << "QtQuick2Plugin")
            << QStringList()
            << QStringList()
            << QStringList()
            << true;

    QTest::newRow("prefer")
        << "prefer/qmldir"
        << ":/foo/bar/"
        << QStringList()
        << QStringList({"doesnotexist|"})
        << QStringList({"FooBarInstance"})
        << QStringList({"Foo|Foo.qml|1|0|false"})
        << QStringList()
        << QStringList()
        << false;

    QTest::newRow("two-prefer")
        << "two-prefer/qmldir"
        << ":/foo/bar/"
        << QStringList({"qmldir:4: only one prefer directive may be defined in a qmldir file"})
        << QStringList({"doesnotexist|"})
        << QStringList({"FooBarInstance"})
        << QStringList({"Foo|Foo.qml|1|0|false"})
        << QStringList()
        << QStringList()
        << false;

    QTest::newRow("prefer-no-slash")
        << "prefer-no-slash/qmldir"
        << QString()
        << QStringList({"qmldir:3: the preferred directory has to end with a '/'"})
        << QStringList({"doesnotexist|"})
        << QStringList({"FooBarInstance"})
        << QStringList({"Foo|Foo.qml|1|0|false"})
        << QStringList()
        << QStringList()
        << false;

    QTest::newRow("versioned-internal")
        << "versioned-internal/qmldir"
        << QString()
        << QStringList()
        << QStringList()
        << QStringList()
        << QStringList({"InternalType|InternalType.qml|1|0|true"})
        << QStringList()
        << QStringList()
        << false;
}

void tst_qqmldirparser::parse()
{
    QFETCH(QString, file);
    QFETCH(QStringList, errors);
    QFETCH(QStringList, plugins);
    QFETCH(QStringList, classnames);
    QFETCH(QStringList, components);
    QFETCH(QStringList, scripts);
    QFETCH(QStringList, dependencies);
    QFETCH(bool, designerSupported);

    QFile f(testFile(file));
    f.open(QIODevice::ReadOnly);

    QQmlDirParser p;
    p.parse(f.readAll());

    if (errors.isEmpty()) {
        QCOMPARE(p.hasError(), false);
    } else {
        QCOMPARE(p.hasError(), true);
        QCOMPARE(toStringList(p.errors("qmldir")), errors);
    }

    QCOMPARE(toStringList(p.plugins()), plugins);
    QCOMPARE(p.classNames(), classnames);
    QCOMPARE(toStringList(p.components()), components);
    QCOMPARE(toStringList(p.scripts()), scripts);
    QCOMPARE(toStringList(p.dependencies()), dependencies);

    QCOMPARE(p.designerSupported(), designerSupported);

    p.clear();
    QVERIFY(p.typeNamespace().isEmpty());
}

QTEST_MAIN(tst_qqmldirparser)

#include "tst_qqmldirparser.moc"
