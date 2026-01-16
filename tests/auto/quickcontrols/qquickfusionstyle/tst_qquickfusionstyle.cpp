// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <private/qmlutils_p.h>

#include <QtTest/qtest.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>

using namespace Qt::StringLiterals;

class tst_qquickfusionstyle : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickfusionstyle() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void fusionWithMissingPalette();
};

void tst_qquickfusionstyle::fusionWithMissingPalette()
{
    QTest::failOnWarning();
    qmlClearTypeRegistrations();

    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("fusionWithMissingPalette.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> object(component.create());
    QVERIFY(object);

    QMetaObject::invokeMethod(object.get(), "torture");
}

QTEST_MAIN(tst_qquickfusionstyle)

#include "tst_qquickfusionstyle.moc"
