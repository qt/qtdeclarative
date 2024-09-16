// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QDirIterator>
#include <QDebug>
#include <QTest>
#include <QQmlEngine>
#include <QQmlComponent>

#include "testtypes.h"

using namespace Qt::StringLiterals;

class tst_javascript : public QObject
{
    Q_OBJECT

public:
    tst_javascript() { registerTypes(); }

private slots:
    void run_data();
    void run();

private:
    QQmlEngine engine;
};

void tst_javascript::run_data()
{
    QTest::addColumn<QString>("file");
    QDirIterator listing(SRCDIR "/data", QStringList{u"*.qml"_s},
                         QDir::Files | QDir::NoDotAndDotDot);
    while (listing.hasNext()) {
        auto info = listing.nextFileInfo();
        const QString base = info.baseName();
        if (!base.isEmpty() && base.at(0).isLower())
            QTest::newRow(qPrintable(base)) << info.filePath();
    }
}

void tst_javascript::run()
{
    QFETCH(QString, file);
    QQmlComponent c(&engine, file);

    if (c.isError())
        qWarning() << c.errors();
    QVERIFY(!c.isError());

    QScopedPointer<QObject> o(c.create());
    QVERIFY(o);
    QMetaMethod method = o->metaObject()->method(o->metaObject()->indexOfMethod("runtest()"));

    QBENCHMARK {
        method.invoke(o.get());
    }
}

QTEST_MAIN(tst_javascript)

#include "tst_javascript.moc"
