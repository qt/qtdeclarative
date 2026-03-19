// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>
#include <QtQuickTest/quicktest.h>
#include <QtQuick/qquickview.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>

#include <QtLabsStyleKit/private/qqstylekitstyle_p.h>
#include <QtLabsStyleKit/private/qqstylekitreader_p.h>

#define LOAD_STYLEKIT_FILE(fileName) \
    QScopedPointer<QQuickView> view(new QQuickView(nullptr)); \
    view->setSource(testFileUrl(fileName)); \
    view->show(); \
    view->requestActivate(); \
    QVERIFY(QTest::qWaitForWindowActive(view.get()));

#define GET_CONTROL(NAME) \
  QQuickItem * NAME ## Control = view->rootObject()->property(#NAME).value<QQuickItem *>(); \
  QVERIFY(NAME ## Control); \
  auto * NAME = NAME ## Control ->findChild<QQStyleKitReader *>(); \
  QVERIFY(NAME)

class tst_StyleKit : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_StyleKit();

private slots:
    void propagation_variations();
};

tst_StyleKit::tst_StyleKit()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_StyleKit::propagation_variations()
{
    /* Test that variations respect the documented propagation order. That is,
     * anything in a Theme should override anything in a Style. From this follows that
     * properties set on a variation in the Style should be shadowed by the same
     * properties set anywhere in the Theme (e.g. Theme.control will shadow an instance
     * variation in the Style).
     * Also, with respect to propagation, it doesn't matter if a type variation is defined
     * in the Style or the Theme, what matters is from where it's used.
     * If both an instance variation and a type variation are defined in the same Style
     * or Theme, with the same properties, the instance variation takes precedence. */
    LOAD_STYLEKIT_FILE("styleVariations.qml")

    GET_CONTROL(insideFrame);
    QCOMPARE(insideFrame->background()->border()->width(), 4);
    QCOMPARE(insideFrame->background()->radius(), 4);

    GET_CONTROL(outsideFrame);
    QCOMPARE(outsideFrame->background()->border()->width(), 3);
    QCOMPARE(outsideFrame->background()->radius(), 4);

    GET_CONTROL(insideFrame_instanceVariation);
    QCOMPARE(insideFrame_instanceVariation->background()->border()->width(), 5);
    QCOMPARE(insideFrame_instanceVariation->background()->radius(), 8);

    GET_CONTROL(outsideFrame_instanceVariation);
    QCOMPARE(outsideFrame_instanceVariation->background()->border()->width(), 5);
    QCOMPARE(outsideFrame_instanceVariation->background()->radius(), 8);
}

QTEST_MAIN(tst_StyleKit)

#include "tst_stylekit.moc"
