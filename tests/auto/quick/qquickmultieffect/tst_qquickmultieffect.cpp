// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>

#include <QtQml/qqmlengine.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>

class tst_qquickmultieffect : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_qquickmultieffect();

private slots:
    void sourceDeletedBeforeReassign();
};

tst_qquickmultieffect::tst_qquickmultieffect()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_qquickmultieffect::sourceDeletedBeforeReassign()
{
    QQmlEngine engine;
    QQmlComponent component(&engine, testFileUrl("sourceGarbageCollected.qml"));
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));

    std::unique_ptr<QObject> effect(component.create());
    QVERIFY(effect);

    QCOMPARE_NE(effect->property("source").value<QObject*>(), nullptr);
    QCOMPARE_NE(effect->property("maskSource").value<QObject*>(), nullptr);

    engine.collectGarbage();

    QTRY_COMPARE(effect->property("source").value<QObject*>(), nullptr);
    QTRY_COMPARE(effect->property("maskSource").value<QObject*>(), nullptr);
}

QTEST_MAIN(tst_qquickmultieffect)

#include "tst_qquickmultieffect.moc"
