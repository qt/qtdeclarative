// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <QtQml/qjsengine.h>

class tst_qv4regexp : public QObject
{
    Q_OBJECT

private slots:
    void catchJitFail();
};

void tst_qv4regexp::catchJitFail()
{
    QJSEngine engine;
    QJSValue result = engine.evaluate(QLatin1String(
            "var prevString = \" ok\";"
            "var r = /^(\\s*)(([\\)\\]}]?\\s*)*([\\)\\]]\\s*))?;/.exec(prevString);"
            "r === null;"), QLatin1String("regexptest.js"));
    QVERIFY(result.isBool());
    QVERIFY(result.toBool());
}

QTEST_MAIN(tst_qv4regexp)

#include "tst_qv4regexp.moc"
