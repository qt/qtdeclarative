// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QDebug>
#include <QDir>
#include <QFile>

class tst_parserstress : public QObject
{
    Q_OBJECT
public:
    tst_parserstress() {}

private slots:
    void init();
    void ecmascript_data();
    void ecmascript();

private:
    static QFileInfoList findJSFiles(const QDir &);
    QQmlEngine engine;
};

void tst_parserstress::init()
{
    QTest::failOnWarning(QRegularExpression(QStringLiteral(".?")));
}

QFileInfoList tst_parserstress::findJSFiles(const QDir &d)
{
    QFileInfoList rv;

    const QFileInfoList files = d.entryInfoList(QStringList() << QLatin1String("*.js"),
                                    QDir::Files);
    for (const QFileInfo &fileInfo : files) {
        if (fileInfo.fileName() == "browser.js")
            continue;
        rv << fileInfo;
    }

    const QStringList dirs = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot |
                                   QDir::NoSymLinks);
    for (const QString &dir : dirs) {
        QDir sub = d;
        sub.cd(dir);
        rv << findJSFiles(sub);
    }

    return rv;
}

struct IgnoredWarning {
    QString ignorePattern;
    int timesToIgnore;
};

void tst_parserstress::ecmascript_data()
{
    QString testDataDir = QFileInfo(QFINDTESTDATA("tests/shell.js")).absolutePath();
    QVERIFY2(!testDataDir.isEmpty(), qPrintable("Cannot find testDataDir!"));

    QDir dir(testDataDir);
    const QFileInfoList files = findJSFiles(dir);

    // We only care that the parser can parse, and warnings shouldn't affect that.
    QHash<QString, IgnoredWarning> warningsToIgnore = {
        { "15.1.2.2-1.js", { "Variable \"POWER\" is used before its declaration at .*", 48 } },
        { "15.1.2.4.js", { "Variable \"index\" is used before its declaration at .*", 6 } },
        { "15.1.2.5-1.js", { "Variable \"index\" is used before its declaration at .*", 6 } },
        { "15.1.2.5-2.js", { "Variable \"index\" is used before its declaration at .*", 6 } },
        { "15.1.2.5-3.js", { "Variable \"index\" is used before its declaration at .*", 6 } },
        { "12.6.3-4.js", { "Variable \"value\" is used before its declaration at .*", 4 } },
        { "try-006.js", { "Variable \"EXCEPTION_STRING\" is used before its declaration at .*", 2 } },
        { "try-007.js", { "Variable \"EXCEPTION_STRING\" is used before its declaration at .*", 2 } },
        { "try-008.js", { "Variable \"INVALID_INTEGER_VALUE\" is used before its declaration at .*", 2 } },
        { "regress-94506.js", { "Variable \"arguments\" is used before its declaration at .*", 2 } },
    };

    QTest::addColumn<QFileInfo>("fileInfo");
    QTest::addColumn<IgnoredWarning>("warningToIgnore");
    for (const QFileInfo &fileInfo : files)
        QTest::newRow(qPrintable(fileInfo.absoluteFilePath())) << fileInfo << warningsToIgnore.value(fileInfo.fileName());
}

void tst_parserstress::ecmascript()
{
    QFETCH(QFileInfo, fileInfo);
    QFETCH(IgnoredWarning, warningToIgnore);

    QFile f(fileInfo.absoluteFilePath());
    QVERIFY(f.open(QIODevice::ReadOnly));

    QByteArray data = f.readAll();

    QVERIFY(!data.isEmpty());

    QString dataStr = QString::fromUtf8(data);

    QString qml = "import QtQuick 2.0\n";
            qml+= "\n";
            qml+= "QtObject {\n";
            qml+= "    property int test\n";
            qml+= "    test: {\n";
            qml+= dataStr + "\n";
            qml+= "        return 1;\n";
            qml+= "    }\n";
            qml+= "    function stress() {\n";
            qml+= dataStr;
            qml+= "    }\n";
            qml+= "}\n";

    QByteArray qmlData = qml.toUtf8();

    if (!warningToIgnore.ignorePattern.isEmpty()) {
        for (int i = 0; i < warningToIgnore.timesToIgnore; ++i)
            QTest::ignoreMessage(QtWarningMsg, QRegularExpression(warningToIgnore.ignorePattern));
    }

    QQmlComponent component(&engine);

    component.setData(qmlData, QUrl());

    if (fileInfo.fileName() == QLatin1String("regress-352044-02-n.js")) {
        QVERIFY(component.isError());

        QCOMPARE(component.errors().size(), 2);

        QCOMPARE(component.errors().at(0).description(), QString("Expected token `;'"));
        QCOMPARE(component.errors().at(0).line(), 66);

        QCOMPARE(component.errors().at(1).description(), QString("Expected token `;'"));
        QCOMPARE(component.errors().at(1).line(), 142);

    } else {
        QVERIFY2(!component.isError(), qPrintable(component.errorString()));
    }
}


QTEST_MAIN(tst_parserstress)

#include "tst_parserstress.moc"
