// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qtest.h>
#include <QtQuick>
#include <QtQuick/private/qquickrectangle_p.h>
#include <QDebug>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <private/qguiapplication_p.h>
#include <qpa/qplatformaccessibility.h>
#include <qpa/qplatformintegration.h>
#include <QtQml/QQmlProperty>

struct TestAccessibility {
    static bool setActive(bool active)
    {
         QPlatformAccessibility *qpaA11Y = platformAccessibility();
         if (qpaA11Y)
            qpaA11Y->setActive(active);
        return qpaA11Y ? true : false;
    }

private:
    static QPlatformAccessibility *platformAccessibility()
    {
        QPlatformIntegration *pfIntegration = QGuiApplicationPrivate::platformIntegration();
        return pfIntegration ? pfIntegration->accessibility() : nullptr;
    }

};


class tst_bench_accessibility : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_bench_accessibility();

private slots:
    void moveItem_data();
    void moveItem();

public slots:
    void initTestCase() override {
        QQmlDataTest::initTestCase();
    }

};

tst_bench_accessibility::tst_bench_accessibility()
    : QQmlDataTest(QUICK_TEST_SOURCE_DIR)
{
}

void tst_bench_accessibility::moveItem_data()
{
    QTest::addColumn<bool>("activateAccessibility");
    QTest::addColumn<QAccessible::Role>("role");
    QTest::newRow("!isActive") << false << QAccessible::NoRole;
    QTest::newRow("!isActive,Button") << false << QAccessible::Button;
    QTest::newRow("isActive") << true << QAccessible::NoRole;
    QTest::newRow("isActive,Button") << true << QAccessible::Button;
}

void tst_bench_accessibility::moveItem()
{
    QFETCH(bool, activateAccessibility);
    QFETCH(QAccessible::Role, role);

    QQuickView window;
    window.setBaseSize(QSize(400, 400));
    window.setSource(testFileUrl("moveItem.qml"));
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QQuickRectangle *rect = window.rootObject()->findChild<QQuickRectangle *>("Cancel");
    if (role != QAccessible::NoRole) {
        QQmlProperty p(rect, "Accessible.role", qmlContext(rect));
        p.write(role);
    }

    QVERIFY(TestAccessibility::setActive(activateAccessibility));
    QVERIFY(QAccessible::isActive() == activateAccessibility);
    QBENCHMARK {
        for (int y = 0; y < 100; ++y) {
            for (int x = 0; x < 100; ++x) {
                rect->setPosition(QPointF(x, y));
            }
        }
    }
}


QTEST_MAIN(tst_bench_accessibility)
#include "tst_bench_accessibility.moc"
