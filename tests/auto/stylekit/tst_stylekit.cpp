// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>
#include <QtCore/qcoreapplication.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQuick/private/qquickrectangle_p.h>
#include <QtQuick/private/qquicktransition_p.h>
#include <QtQuick/private/qquickimage_p.h>

#include <QtLabsStyleKit/private/qqstylekit_p.h>
#include <QtLabsStyleKit/private/qqstylekitstyle_p.h>
#include <QtLabsStyleKit/private/qqstylekitreader_p.h>

using namespace Qt::StringLiterals;

#define LOAD_STYLE(fileName) \
    QQmlEngine engine; \
    QQmlComponent component(&engine, QUrl::fromLocalFile( \
            QStringLiteral(QT_QMLTEST_DATADIR) \
            + QLatin1Char('/') + QStringLiteral(#fileName))); \
    QVERIFY2(component.isReady(), qPrintable(component.errorString())); \
    QScopedPointer<QQStyleKitStyle> style(qobject_cast<QQStyleKitStyle *>(component.create())); \
    QVERIFY(style);

static QString objectNameOf(const QObject *obj)
{
    return obj ? obj->objectName() : QString();
}

static QString objectNameOfCreated(QQmlComponent *component)
{
    if (!component)
        return QString();
    QScopedPointer<QObject> obj(component->create());
    return objectNameOf(obj.data());
}

class tst_StyleKit : public QObject
{
    Q_OBJECT

private slots:
    void propagationStyleAndFallback();
    void propagationThemeAndStyle();
    void propagationThemeStyleFallbackStyle_data();
    void propagationThemeStyleFallbackStyle();
    void controlProperties_data();
    void controlProperties();
};

void tst_StyleKit::propagationStyleAndFallback()
{
    LOAD_STYLE(propagationStyleAndFallback.qml)
    QQStyleKitReader reader;
    reader.setExplicitStyle(style.data());
    reader.setControlType(QQStyleKitReader::ControlType::Button);
    QCOMPARE(reader.background()->leftMargin(), 1);
    QCOMPARE(reader.leftPadding(), 2);
    QCOMPARE(reader.rightPadding(), 3);
    QCOMPARE(reader.topPadding(), 4);
    QCOMPARE(reader.bottomPadding(), 5);
    QCOMPARE(reader.spacing(), 6);

    reader.setControlType(QQStyleKitReader::ControlType::RadioButton);
    QCOMPARE(reader.background()->leftMargin(), 1);
    QCOMPARE(reader.leftPadding(), 2);
    QCOMPARE(reader.rightPadding(), 2);
    QCOMPARE(reader.topPadding(), 4);
    QCOMPARE(reader.bottomPadding(), 5);
    QCOMPARE(reader.spacing(), 5);

    reader.setControlType(QQStyleKitReader::ControlType::Slider);
    QCOMPARE(reader.background()->leftMargin(), 1);
    QCOMPARE(reader.leftPadding(), 1);
    QCOMPARE(reader.rightPadding(), 1);
    QCOMPARE(reader.topPadding(), 4);
    QCOMPARE(reader.bottomPadding(), 4);
    QCOMPARE(reader.spacing(), 4);
}

void tst_StyleKit::propagationThemeAndStyle()
{
    LOAD_STYLE(propagationThemeAndStyle.qml)
    QQStyleKitReader reader;
    reader.setExplicitStyle(style.data());
    reader.setControlType(QQStyleKitReader::ControlType::Button);
    QCOMPARE(reader.background()->leftMargin(), 1);
    QCOMPARE(reader.leftPadding(), 2);
    QCOMPARE(reader.rightPadding(), 3);
    QCOMPARE(reader.topPadding(), 4);
    QCOMPARE(reader.bottomPadding(), 5);
    QCOMPARE(reader.spacing(), 6);

    reader.setControlType(QQStyleKitReader::ControlType::RadioButton);
    QCOMPARE(reader.background()->leftMargin(), 1);
    QCOMPARE(reader.leftPadding(), 2);
    QCOMPARE(reader.rightPadding(), 2);
    QCOMPARE(reader.topPadding(), 4);
    QCOMPARE(reader.bottomPadding(), 5);
    QCOMPARE(reader.spacing(), 5);

    reader.setControlType(QQStyleKitReader::ControlType::Slider);
    QCOMPARE(reader.background()->leftMargin(), 1);
    QCOMPARE(reader.leftPadding(), 1);
    QCOMPARE(reader.rightPadding(), 1);
    QCOMPARE(reader.topPadding(), 4);
    QCOMPARE(reader.bottomPadding(), 4);
    QCOMPARE(reader.spacing(), 4);
}

void tst_StyleKit::propagationThemeStyleFallbackStyle_data()
{
    // The levels (see propagationThemeStyleFallbackStyle.qml) that apply to each control type
    QTest::addColumn<QQStyleKitReader::ControlType>("controlType");
    QTest::addColumn<QList<int>>("levels");
    QTest::newRow("Button") << QQStyleKitReader::ControlType::Button << QList<int>{1, 2, 3, 4, 5, 6, 7, 8, 9};
    QTest::newRow("RadioButton") << QQStyleKitReader::ControlType::RadioButton << QList<int>{1, 2, 4, 5, 7, 8};
    QTest::newRow("Slider") << QQStyleKitReader::ControlType::Slider << QList<int>{1, 4, 7};
}

void tst_StyleKit::propagationThemeStyleFallbackStyle()
{
    // This test is bascially testing the same as propagationStyleAndFallback()
    // and propagationThemeAndStyle() combined, but with a more exhaustive set of
    // properties, and with a more complex hierarchy of theme, style, and fallback style.
    // It checks that every color and number property propagates correctly through
    // the theme, the style, and the fallback style, for different control types.
    // See propagationThemeStyleFallbackStyle.qml for how the values are constructed.
    QFETCH(QQStyleKitReader::ControlType, controlType);
    QFETCH(QList<int>, levels);

    // In the QML file, each property is assigned a value on every level from 1 up to
    // a property specific level, marked with a "// final assignment" comment. Above
    // that level the property is unset, so reading it should fall through to a lower
    // level. The helpers below take the level of the final assignment, together with
    // the property's id, and return the value we expect to read. E.g bottomPadding has
    // id 6, and its final assignment on level 6, so we check it with num(6, 6). For a
    // Button, we expect to read the value from level 6, which is 66. But for a
    // RadioButton, level 6 (button) doesn't apply, so we expect to read 65 from level 5
    // instead. And for a Slider, we expect to read 64 from level 4.

    // Returns the most specific level, up to and including finalAssignmentLevel,
    // that applies to the control type
    const auto level = [&levels](int finalAssignmentLevel) {
        int result = 0;
        for (int l : levels) {
            if (l <= finalAssignmentLevel)
                result = l;
        }
        return result;
    };
    const auto num = [&](int id, int finalAssignmentLevel) {
        return id * 10 + level(finalAssignmentLevel);
    };
    const auto frac = [&](int id, int finalAssignmentLevel) {
        return id / 1000.0 + level(finalAssignmentLevel) / 10000.0;
    };
    const auto color = [&](int colorId, int finalAssignmentLevel) {
        return QColor(colorId * 5, level(finalAssignmentLevel) * 25, (colorId * 37) % 256);
    };

    LOAD_STYLE(propagationThemeStyleFallbackStyle.qml)
    QQStyleKitReader reader;
    reader.setExplicitStyle(style.data());
    reader.setControlType(controlType);

    QCOMPARE(reader.spacing(), num(1, 1));
    QCOMPARE(reader.padding(), num(2, 2));
    QCOMPARE(reader.leftPadding(), num(3, 3));
    QCOMPARE(reader.rightPadding(), num(4, 4));
    QCOMPARE(reader.topPadding(), num(5, 5));
    QCOMPARE(reader.bottomPadding(), num(6, 6));

    {
        // background
        const QQStyleKitDelegateProperties *d = reader.background();
        QVERIFY(d);
        QCOMPARE(d->width(), num(7, 7));
        QCOMPARE(d->height(), num(8, 8));
        QCOMPARE(d->minimumWidth(), num(9, 9));
        QCOMPARE(d->minimumHeight(), num(10, 1));
        QCOMPARE(d->margins(), num(11, 2));
        QCOMPARE(d->leftMargin(), num(12, 3));
        QCOMPARE(d->rightMargin(), num(13, 4));
        QCOMPARE(d->topMargin(), num(14, 5));
        QCOMPARE(d->bottomMargin(), num(15, 6));
        QCOMPARE(d->radius(), num(16, 7));
        QCOMPARE(d->topLeftRadius(), num(17, 8));
        QCOMPARE(d->topRightRadius(), num(18, 9));
        QCOMPARE(d->bottomLeftRadius(), num(19, 7));
        QCOMPARE(d->bottomRightRadius(), num(20, 7));
        QCOMPARE(d->rotation(), num(21, 3));
        QCOMPARE(d->scale(), 1 + frac(22, 4));
        QCOMPARE(d->opacity(), frac(23, 5));
        QCOMPARE(d->color(), color(1, 6));
        QCOMPARE(d->border()->width(), num(25, 7));
        QCOMPARE(d->border()->color(), color(2, 8));
        QCOMPARE(d->shadow()->color(), color(3, 9));
        QCOMPARE(d->shadow()->opacity(), frac(28, 1));
        QCOMPARE(d->shadow()->scale(), 1 + frac(29, 2));
        QCOMPARE(d->shadow()->verticalOffset(), num(30, 3));
        QCOMPARE(d->shadow()->horizontalOffset(), num(31, 4));
        QCOMPARE(d->shadow()->blur(), num(32, 5));
        QCOMPARE(d->image()->color(), color(4, 6));
    }

    {
        // handle
        const QQStyleKitDelegateProperties *d = reader.handle();
        QVERIFY(d);
        QCOMPARE(d->width(), num(34, 7));
        QCOMPARE(d->height(), num(35, 8));
        QCOMPARE(d->minimumWidth(), num(36, 9));
        QCOMPARE(d->minimumHeight(), num(37, 1));
        QCOMPARE(d->margins(), num(38, 2));
        QCOMPARE(d->leftMargin(), num(39, 3));
        QCOMPARE(d->rightMargin(), num(40, 4));
        QCOMPARE(d->topMargin(), num(41, 5));
        QCOMPARE(d->bottomMargin(), num(42, 6));
        QCOMPARE(d->radius(), num(43, 7));
        QCOMPARE(d->topLeftRadius(), num(44, 8));
        QCOMPARE(d->topRightRadius(), num(45, 9));
        QCOMPARE(d->bottomLeftRadius(), num(46, 7));
        QCOMPARE(d->bottomRightRadius(), num(47, 7));
        QCOMPARE(d->rotation(), num(48, 3));
        QCOMPARE(d->scale(), 1 + frac(49, 4));
        QCOMPARE(d->opacity(), frac(50, 5));
        QCOMPARE(d->color(), color(5, 6));
        QCOMPARE(d->border()->width(), num(52, 7));
        QCOMPARE(d->border()->color(), color(6, 8));
        QCOMPARE(d->shadow()->color(), color(7, 9));
        QCOMPARE(d->shadow()->opacity(), frac(55, 1));
        QCOMPARE(d->shadow()->scale(), 1 + frac(56, 2));
        QCOMPARE(d->shadow()->verticalOffset(), num(57, 3));
        QCOMPARE(d->shadow()->horizontalOffset(), num(58, 4));
        QCOMPARE(d->shadow()->blur(), num(59, 5));
        QCOMPARE(d->image()->color(), color(8, 6));
    }

    {
        // handle.first
        const QQStyleKitDelegateProperties *d = reader.handle()->first();
        QVERIFY(d);
        QCOMPARE(d->width(), num(61, 7));
        QCOMPARE(d->height(), num(62, 8));
        QCOMPARE(d->minimumWidth(), num(63, 9));
        QCOMPARE(d->minimumHeight(), num(64, 1));
        QCOMPARE(d->margins(), num(65, 2));
        QCOMPARE(d->leftMargin(), num(66, 3));
        QCOMPARE(d->rightMargin(), num(67, 4));
        QCOMPARE(d->topMargin(), num(68, 5));
        QCOMPARE(d->bottomMargin(), num(69, 6));
        QCOMPARE(d->radius(), num(70, 7));
        QCOMPARE(d->topLeftRadius(), num(71, 8));
        QCOMPARE(d->topRightRadius(), num(72, 9));
        QCOMPARE(d->bottomLeftRadius(), num(73, 7));
        QCOMPARE(d->bottomRightRadius(), num(74, 7));
        QCOMPARE(d->rotation(), num(75, 3));
        QCOMPARE(d->scale(), 1 + frac(76, 4));
        QCOMPARE(d->opacity(), frac(77, 5));
        QCOMPARE(d->color(), color(9, 6));
        QCOMPARE(d->border()->width(), num(79, 7));
        QCOMPARE(d->border()->color(), color(10, 8));
        QCOMPARE(d->shadow()->color(), color(11, 9));
        QCOMPARE(d->shadow()->opacity(), frac(82, 1));
        QCOMPARE(d->shadow()->scale(), 1 + frac(83, 2));
        QCOMPARE(d->shadow()->verticalOffset(), num(84, 3));
        QCOMPARE(d->shadow()->horizontalOffset(), num(85, 4));
        QCOMPARE(d->shadow()->blur(), num(86, 5));
        QCOMPARE(d->image()->color(), color(12, 6));
    }

    {
        // handle.second
        const QQStyleKitDelegateProperties *d = reader.handle()->second();
        QVERIFY(d);
        QCOMPARE(d->width(), num(88, 7));
        QCOMPARE(d->height(), num(89, 8));
        QCOMPARE(d->minimumWidth(), num(90, 9));
        QCOMPARE(d->minimumHeight(), num(91, 1));
        QCOMPARE(d->margins(), num(92, 2));
        QCOMPARE(d->leftMargin(), num(93, 3));
        QCOMPARE(d->rightMargin(), num(94, 4));
        QCOMPARE(d->topMargin(), num(95, 5));
        QCOMPARE(d->bottomMargin(), num(96, 6));
        QCOMPARE(d->radius(), num(97, 7));
        QCOMPARE(d->topLeftRadius(), num(98, 8));
        QCOMPARE(d->topRightRadius(), num(99, 9));
        QCOMPARE(d->bottomLeftRadius(), num(100, 7));
        QCOMPARE(d->bottomRightRadius(), num(101, 7));
        QCOMPARE(d->rotation(), num(102, 3));
        QCOMPARE(d->scale(), 1 + frac(103, 4));
        QCOMPARE(d->opacity(), frac(104, 5));
        QCOMPARE(d->color(), color(13, 6));
        QCOMPARE(d->border()->width(), num(106, 7));
        QCOMPARE(d->border()->color(), color(14, 8));
        QCOMPARE(d->shadow()->color(), color(15, 9));
        QCOMPARE(d->shadow()->opacity(), frac(109, 1));
        QCOMPARE(d->shadow()->scale(), 1 + frac(110, 2));
        QCOMPARE(d->shadow()->verticalOffset(), num(111, 3));
        QCOMPARE(d->shadow()->horizontalOffset(), num(112, 4));
        QCOMPARE(d->shadow()->blur(), num(113, 5));
        QCOMPARE(d->image()->color(), color(16, 6));
    }

    {
        // indicator
        const QQStyleKitDelegateProperties *d = reader.indicator();
        QVERIFY(d);
        QCOMPARE(d->width(), num(115, 7));
        QCOMPARE(d->height(), num(116, 8));
        QCOMPARE(d->minimumWidth(), num(117, 9));
        QCOMPARE(d->minimumHeight(), num(118, 1));
        QCOMPARE(d->margins(), num(119, 2));
        QCOMPARE(d->leftMargin(), num(120, 3));
        QCOMPARE(d->rightMargin(), num(121, 4));
        QCOMPARE(d->topMargin(), num(122, 5));
        QCOMPARE(d->bottomMargin(), num(123, 6));
        QCOMPARE(d->radius(), num(124, 7));
        QCOMPARE(d->topLeftRadius(), num(125, 8));
        QCOMPARE(d->topRightRadius(), num(126, 9));
        QCOMPARE(d->bottomLeftRadius(), num(127, 7));
        QCOMPARE(d->bottomRightRadius(), num(128, 7));
        QCOMPARE(d->rotation(), num(129, 3));
        QCOMPARE(d->scale(), 1 + frac(130, 4));
        QCOMPARE(d->opacity(), frac(131, 5));
        QCOMPARE(d->color(), color(17, 6));
        QCOMPARE(d->border()->width(), num(133, 7));
        QCOMPARE(d->border()->color(), color(18, 8));
        QCOMPARE(d->shadow()->color(), color(19, 9));
        QCOMPARE(d->shadow()->opacity(), frac(136, 1));
        QCOMPARE(d->shadow()->scale(), 1 + frac(137, 2));
        QCOMPARE(d->shadow()->verticalOffset(), num(138, 3));
        QCOMPARE(d->shadow()->horizontalOffset(), num(139, 4));
        QCOMPARE(d->shadow()->blur(), num(140, 5));
        QCOMPARE(d->image()->color(), color(20, 6));
    }

    {
        // indicator.foreground
        const QQStyleKitDelegateProperties *d = reader.indicator()->foreground();
        QVERIFY(d);
        QCOMPARE(d->width(), num(142, 7));
        QCOMPARE(d->height(), num(143, 8));
        QCOMPARE(d->minimumWidth(), num(144, 9));
        QCOMPARE(d->minimumHeight(), num(145, 1));
        QCOMPARE(d->margins(), num(146, 2));
        QCOMPARE(d->leftMargin(), num(147, 3));
        QCOMPARE(d->rightMargin(), num(148, 4));
        QCOMPARE(d->topMargin(), num(149, 5));
        QCOMPARE(d->bottomMargin(), num(150, 6));
        QCOMPARE(d->radius(), num(151, 7));
        QCOMPARE(d->topLeftRadius(), num(152, 8));
        QCOMPARE(d->topRightRadius(), num(153, 9));
        QCOMPARE(d->bottomLeftRadius(), num(154, 7));
        QCOMPARE(d->bottomRightRadius(), num(155, 7));
        QCOMPARE(d->rotation(), num(156, 3));
        QCOMPARE(d->scale(), 1 + frac(157, 4));
        QCOMPARE(d->opacity(), frac(158, 5));
        QCOMPARE(d->color(), color(21, 6));
        QCOMPARE(d->border()->width(), num(160, 7));
        QCOMPARE(d->border()->color(), color(22, 8));
        QCOMPARE(d->shadow()->color(), color(23, 9));
        QCOMPARE(d->shadow()->opacity(), frac(163, 1));
        QCOMPARE(d->shadow()->scale(), 1 + frac(164, 2));
        QCOMPARE(d->shadow()->verticalOffset(), num(165, 3));
        QCOMPARE(d->shadow()->horizontalOffset(), num(166, 4));
        QCOMPARE(d->shadow()->blur(), num(167, 5));
        QCOMPARE(d->image()->color(), color(24, 6));
    }

    {
        // indicator.first
        const QQStyleKitDelegateProperties *d = reader.indicator()->first();
        QVERIFY(d);
        QCOMPARE(d->width(), num(169, 7));
        QCOMPARE(d->height(), num(170, 8));
        QCOMPARE(d->minimumWidth(), num(171, 9));
        QCOMPARE(d->minimumHeight(), num(172, 1));
        QCOMPARE(d->margins(), num(173, 2));
        QCOMPARE(d->leftMargin(), num(174, 3));
        QCOMPARE(d->rightMargin(), num(175, 4));
        QCOMPARE(d->topMargin(), num(176, 5));
        QCOMPARE(d->bottomMargin(), num(177, 6));
        QCOMPARE(d->radius(), num(178, 7));
        QCOMPARE(d->topLeftRadius(), num(179, 8));
        QCOMPARE(d->topRightRadius(), num(180, 9));
        QCOMPARE(d->bottomLeftRadius(), num(181, 7));
        QCOMPARE(d->bottomRightRadius(), num(182, 7));
        QCOMPARE(d->rotation(), num(183, 3));
        QCOMPARE(d->scale(), 1 + frac(184, 4));
        QCOMPARE(d->opacity(), frac(185, 5));
        QCOMPARE(d->color(), color(25, 6));
        QCOMPARE(d->border()->width(), num(187, 7));
        QCOMPARE(d->border()->color(), color(26, 8));
        QCOMPARE(d->shadow()->color(), color(27, 9));
        QCOMPARE(d->shadow()->opacity(), frac(190, 1));
        QCOMPARE(d->shadow()->scale(), 1 + frac(191, 2));
        QCOMPARE(d->shadow()->verticalOffset(), num(192, 3));
        QCOMPARE(d->shadow()->horizontalOffset(), num(193, 4));
        QCOMPARE(d->shadow()->blur(), num(194, 5));
        QCOMPARE(d->image()->color(), color(28, 6));
    }

    {
        // indicator.first.foreground
        const QQStyleKitDelegateProperties *d = reader.indicator()->first()->foreground();
        QVERIFY(d);
        QCOMPARE(d->width(), num(196, 7));
        QCOMPARE(d->height(), num(197, 8));
        QCOMPARE(d->minimumWidth(), num(198, 9));
        QCOMPARE(d->minimumHeight(), num(199, 1));
        QCOMPARE(d->margins(), num(200, 2));
        QCOMPARE(d->leftMargin(), num(201, 3));
        QCOMPARE(d->rightMargin(), num(202, 4));
        QCOMPARE(d->topMargin(), num(203, 5));
        QCOMPARE(d->bottomMargin(), num(204, 6));
        QCOMPARE(d->radius(), num(205, 7));
        QCOMPARE(d->topLeftRadius(), num(206, 8));
        QCOMPARE(d->topRightRadius(), num(207, 9));
        QCOMPARE(d->bottomLeftRadius(), num(208, 7));
        QCOMPARE(d->bottomRightRadius(), num(209, 7));
        QCOMPARE(d->rotation(), num(210, 3));
        QCOMPARE(d->scale(), 1 + frac(211, 4));
        QCOMPARE(d->opacity(), frac(212, 5));
        QCOMPARE(d->color(), color(29, 6));
        QCOMPARE(d->border()->width(), num(214, 7));
        QCOMPARE(d->border()->color(), color(30, 8));
        QCOMPARE(d->shadow()->color(), color(31, 9));
        QCOMPARE(d->shadow()->opacity(), frac(217, 1));
        QCOMPARE(d->shadow()->scale(), 1 + frac(218, 2));
        QCOMPARE(d->shadow()->verticalOffset(), num(219, 3));
        QCOMPARE(d->shadow()->horizontalOffset(), num(220, 4));
        QCOMPARE(d->shadow()->blur(), num(221, 5));
        QCOMPARE(d->image()->color(), color(32, 6));
    }

    {
        // indicator.second
        const QQStyleKitDelegateProperties *d = reader.indicator()->second();
        QVERIFY(d);
        QCOMPARE(d->width(), num(223, 7));
        QCOMPARE(d->height(), num(224, 8));
        QCOMPARE(d->minimumWidth(), num(225, 9));
        QCOMPARE(d->minimumHeight(), num(226, 1));
        QCOMPARE(d->margins(), num(227, 2));
        QCOMPARE(d->leftMargin(), num(228, 3));
        QCOMPARE(d->rightMargin(), num(229, 4));
        QCOMPARE(d->topMargin(), num(230, 5));
        QCOMPARE(d->bottomMargin(), num(231, 6));
        QCOMPARE(d->radius(), num(232, 7));
        QCOMPARE(d->topLeftRadius(), num(233, 8));
        QCOMPARE(d->topRightRadius(), num(234, 9));
        QCOMPARE(d->bottomLeftRadius(), num(235, 7));
        QCOMPARE(d->bottomRightRadius(), num(236, 7));
        QCOMPARE(d->rotation(), num(237, 3));
        QCOMPARE(d->scale(), 1 + frac(238, 4));
        QCOMPARE(d->opacity(), frac(239, 5));
        QCOMPARE(d->color(), color(33, 6));
        QCOMPARE(d->border()->width(), num(241, 7));
        QCOMPARE(d->border()->color(), color(34, 8));
        QCOMPARE(d->shadow()->color(), color(35, 9));
        QCOMPARE(d->shadow()->opacity(), frac(244, 1));
        QCOMPARE(d->shadow()->scale(), 1 + frac(245, 2));
        QCOMPARE(d->shadow()->verticalOffset(), num(246, 3));
        QCOMPARE(d->shadow()->horizontalOffset(), num(247, 4));
        QCOMPARE(d->shadow()->blur(), num(248, 5));
        QCOMPARE(d->image()->color(), color(36, 6));
    }

    {
        // indicator.second.foreground
        const QQStyleKitDelegateProperties *d = reader.indicator()->second()->foreground();
        QVERIFY(d);
        QCOMPARE(d->width(), num(250, 7));
        QCOMPARE(d->height(), num(251, 8));
        QCOMPARE(d->minimumWidth(), num(252, 9));
        QCOMPARE(d->minimumHeight(), num(253, 1));
        QCOMPARE(d->margins(), num(254, 2));
        QCOMPARE(d->leftMargin(), num(255, 3));
        QCOMPARE(d->rightMargin(), num(256, 4));
        QCOMPARE(d->topMargin(), num(257, 5));
        QCOMPARE(d->bottomMargin(), num(258, 6));
        QCOMPARE(d->radius(), num(259, 7));
        QCOMPARE(d->topLeftRadius(), num(260, 8));
        QCOMPARE(d->topRightRadius(), num(261, 9));
        QCOMPARE(d->bottomLeftRadius(), num(262, 7));
        QCOMPARE(d->bottomRightRadius(), num(263, 7));
        QCOMPARE(d->rotation(), num(264, 3));
        QCOMPARE(d->scale(), 1 + frac(265, 4));
        QCOMPARE(d->opacity(), frac(266, 5));
        QCOMPARE(d->color(), color(37, 6));
        QCOMPARE(d->border()->width(), num(268, 7));
        QCOMPARE(d->border()->color(), color(38, 8));
        QCOMPARE(d->shadow()->color(), color(39, 9));
        QCOMPARE(d->shadow()->opacity(), frac(271, 1));
        QCOMPARE(d->shadow()->scale(), 1 + frac(272, 2));
        QCOMPARE(d->shadow()->verticalOffset(), num(273, 3));
        QCOMPARE(d->shadow()->horizontalOffset(), num(274, 4));
        QCOMPARE(d->shadow()->blur(), num(275, 5));
        QCOMPARE(d->image()->color(), color(40, 6));
    }

    {
        // text
        const QQStyleKitTextProperties *d = reader.text();
        QVERIFY(d);
        QCOMPARE(d->color(), color(41, 7));
        QCOMPARE(d->pointSize(), num(278, 8));
        QCOMPARE(d->padding(), num(279, 9));
        QCOMPARE(d->leftPadding(), num(280, 9));
        QCOMPARE(d->rightPadding(), num(281, 9));
        QCOMPARE(d->topPadding(), num(282, 9));
        QCOMPARE(d->bottomPadding(), num(283, 9));
    }

    {
        // text.first
        const QQStyleKitTextProperties *d = reader.text()->first();
        QVERIFY(d);
        QCOMPARE(d->color(), color(42, 7));
        QCOMPARE(d->pointSize(), num(285, 8));
        QCOMPARE(d->padding(), num(286, 9));
        QCOMPARE(d->leftPadding(), num(287, 9));
        QCOMPARE(d->rightPadding(), num(288, 9));
        QCOMPARE(d->topPadding(), num(289, 9));
        QCOMPARE(d->bottomPadding(), num(290, 9));
    }

    {
        // text.second
        const QQStyleKitTextProperties *d = reader.text()->second();
        QVERIFY(d);
        QCOMPARE(d->color(), color(43, 7));
        QCOMPARE(d->pointSize(), num(292, 8));
        QCOMPARE(d->padding(), num(293, 9));
        QCOMPARE(d->leftPadding(), num(294, 9));
        QCOMPARE(d->rightPadding(), num(295, 9));
        QCOMPARE(d->topPadding(), num(296, 9));
        QCOMPARE(d->bottomPadding(), num(297, 9));
    }
}

void tst_StyleKit::controlProperties_data()
{
    QTest::addColumn<QQSK::State>("state");
    QTest::addColumn<int>("stateIndex");
    QTest::newRow("normal") << QQSK::State(QQSK::StateFlag::Normal) << 0;
    QTest::newRow("pressed") << QQSK::State(QQSK::StateFlag::Pressed) << 1;
    QTest::newRow("hovered") << QQSK::State(QQSK::StateFlag::Hovered) << 2;
    QTest::newRow("focused") << QQSK::State(QQSK::StateFlag::Focused) << 3;
    QTest::newRow("checked") << QQSK::State(QQSK::StateFlag::Checked) << 4;
    QTest::newRow("disabled") << QQSK::State(QQSK::StateFlag::Disabled) << 5;
    QTest::newRow("highlighted") << QQSK::State(QQSK::StateFlag::Highlighted) << 6;
    QTest::newRow("vertical") << QQSK::State(QQSK::StateFlag::Vertical) << 7;
    // Nest all states, except disabled. A disabled control
    // ignores pressed, hovered, highlighted and focused.
    QTest::newRow("nested") << QQSK::State(QQSK::StateFlag::Pressed
            | QQSK::StateFlag::Hovered
            | QQSK::StateFlag::Focused
            | QQSK::StateFlag::Checked
            | QQSK::StateFlag::Highlighted
            | QQSK::StateFlag::Vertical) << 8;
}

void tst_StyleKit::controlProperties()
{
    // Every property in the button (recursively) is assigned a unique value, both
    // in the normal state, in each individual control state, and in one nested
    // state. Check that we read back the same values. Since StyleKit gives each
    // property a unique key and stores their values in hash maps, using unique
    // property values in this test makes it more likely to detect key collisions.
    // The values in the style are constructed like this:
    // - numbers: stateIndex * 10000 + delegateIndex * 100 + propertyIndex
    // - scale and opacity: stateIndex * 0.1 + delegateIndex * 0.01 + propertyIndex * 0.001 (+ 1 for scale)
    // - colors: the green component is: colorIndex * 50 + 10 + stateIndex * 5, where
    //   colorIndex is 0-3 for color, border.color, shadow.color and image.color. The
    //   step of 5 per state keeps the offset below 50, so colors stay unique.
    // - alignment and fillMode: the table index is offset by stateIndex
    // - bools: are inverted for every odd stateIndex
    // - object names and image urls: are prefixed with the state name
    QFETCH(QQSK::State, state);
    QFETCH(int, stateIndex);
    LOAD_STYLE(controlProperties.qml)
    QQStyleKitReader reader;
    reader.setExplicitStyle(style.data());
    reader.setControlTypeAndState(QQStyleKitReader::ControlType::Button, state);

    QString prefix;
    if (stateIndex != 0)
        prefix = QString::fromLatin1(QTest::currentDataTag()) + u'.';

    const auto alignmentAt = [](int index) {
        // Must match the order used in data/controlProperties.qml
        static const Qt::Alignment alignments[] = {
            Qt::AlignLeft | Qt::AlignTop,
            Qt::AlignRight | Qt::AlignBottom,
            Qt::AlignHCenter | Qt::AlignVCenter,
            Qt::AlignJustify | Qt::AlignBaseline,
            Qt::AlignLeft | Qt::AlignBottom,
            Qt::AlignRight | Qt::AlignVCenter,
            Qt::AlignHCenter | Qt::AlignBaseline,
            Qt::AlignJustify | Qt::AlignTop,
            Qt::AlignLeft | Qt::AlignVCenter,
            Qt::AlignRight | Qt::AlignBaseline,
            Qt::AlignHCenter | Qt::AlignTop,
            Qt::AlignJustify | Qt::AlignBottom,
            Qt::AlignLeft | Qt::AlignBaseline,
            Qt::AlignRight | Qt::AlignTop,
            Qt::AlignHCenter | Qt::AlignBottom,
            Qt::AlignJustify | Qt::AlignVCenter,
        };
        return alignments[index % std::size(alignments)];
    };

    const auto fillModeAt = [](int index) {
        static const QQuickImage::FillMode fillModes[] = {
            QQuickImage::Stretch,
            QQuickImage::PreserveAspectFit,
            QQuickImage::PreserveAspectCrop,
            QQuickImage::Tile,
            QQuickImage::TileVertically,
            QQuickImage::TileHorizontally,
            QQuickImage::Pad,
        };
        return fillModes[index % std::size(fillModes)];
    };

    const int n = stateIndex * 10000;
    const qreal f = stateIndex * 0.1;
    const int c = stateIndex * 5;
    const bool flip = stateIndex % 2 == 1;

    QCOMPARE(reader.spacing(), n + 1);
    QCOMPARE(reader.padding(), n + 2);
    QCOMPARE(reader.leftPadding(), n + 3);
    QCOMPARE(reader.rightPadding(), n + 4);
    QCOMPARE(reader.topPadding(), n + 5);
    QCOMPARE(reader.bottomPadding(), n + 6);
    QCOMPARE(objectNameOf(reader.transition()), prefix + u"transition"_s);

    {
        // background
        const QQStyleKitDelegateProperties *d = reader.background();
        QVERIFY(d);
        QCOMPARE(d->width(), n + 101);
        QCOMPARE(d->height(), n + 102);
        QCOMPARE(d->minimumWidth(), n + 103);
        QCOMPARE(d->minimumHeight(), n + 104);
        QCOMPARE(d->margins(), n + 105);
        QCOMPARE(d->leftMargin(), n + 106);
        QCOMPARE(d->rightMargin(), n + 107);
        QCOMPARE(d->topMargin(), n + 108);
        QCOMPARE(d->bottomMargin(), n + 109);
        QCOMPARE(d->radius(), n + 110);
        QCOMPARE(d->topLeftRadius(), n + 111);
        QCOMPARE(d->topRightRadius(), n + 112);
        QCOMPARE(d->bottomLeftRadius(), n + 113);
        QCOMPARE(d->bottomRightRadius(), n + 114);
        QCOMPARE(d->rotation(), n + 115);
        QCOMPARE(d->scale(), f + 1.013);
        QCOMPARE(d->opacity(), f + 0.011);
        QCOMPARE(d->color(), QColor(19, 10 + c, 37));
        QCOMPARE(d->alignment(), alignmentAt(0 + stateIndex));
        QCOMPARE(d->fillWidth(), flip);
        QCOMPARE(d->fillHeight(), !flip);
        QCOMPARE(d->visible(), !flip);
        QCOMPARE(d->clip(), flip);
        QCOMPARE(objectNameOf(d->gradient()), prefix + u"background.gradient"_s);
        QCOMPARE(objectNameOf(d->data()), prefix + u"background.data"_s);
        QCOMPARE(objectNameOfCreated(d->delegate()), prefix + u"background.delegate"_s);
        QCOMPARE(d->border()->width(), n + 116);
        QCOMPARE(d->border()->color(), QColor(19, 60 + c, 128));
        QCOMPARE(d->shadow()->color(), QColor(19, 110 + c, 219));
        QCOMPARE(d->shadow()->opacity(), f + 0.012);
        QCOMPARE(d->shadow()->scale(), f + 1.014);
        QCOMPARE(d->shadow()->verticalOffset(), n + 117);
        QCOMPARE(d->shadow()->horizontalOffset(), n + 118);
        QCOMPARE(d->shadow()->blur(), n + 119);
        QCOMPARE(d->shadow()->visible(), !flip);
        QCOMPARE(objectNameOfCreated(d->shadow()->delegate()), prefix + u"background.shadow.delegate"_s);
        QCOMPARE(d->image()->source(), QUrl(prefix + u"background.png"_s));
        QCOMPARE(d->image()->color(), QColor(19, 160 + c, 54));
        QCOMPARE(d->image()->fillMode(), fillModeAt(0 + stateIndex));
    }

    {
        // handle
        const QQStyleKitDelegateProperties *d = reader.handle();
        QVERIFY(d);
        QCOMPARE(d->width(), n + 201);
        QCOMPARE(d->height(), n + 202);
        QCOMPARE(d->minimumWidth(), n + 203);
        QCOMPARE(d->minimumHeight(), n + 204);
        QCOMPARE(d->margins(), n + 205);
        QCOMPARE(d->leftMargin(), n + 206);
        QCOMPARE(d->rightMargin(), n + 207);
        QCOMPARE(d->topMargin(), n + 208);
        QCOMPARE(d->bottomMargin(), n + 209);
        QCOMPARE(d->radius(), n + 210);
        QCOMPARE(d->topLeftRadius(), n + 211);
        QCOMPARE(d->topRightRadius(), n + 212);
        QCOMPARE(d->bottomLeftRadius(), n + 213);
        QCOMPARE(d->bottomRightRadius(), n + 214);
        QCOMPARE(d->rotation(), n + 215);
        QCOMPARE(d->scale(), f + 1.023);
        QCOMPARE(d->opacity(), f + 0.021);
        QCOMPARE(d->color(), QColor(38, 10 + c, 74));
        QCOMPARE(d->alignment(), alignmentAt(1 + stateIndex));
        QCOMPARE(d->fillWidth(), !flip);
        QCOMPARE(d->fillHeight(), flip);
        QCOMPARE(d->visible(), !flip);
        QCOMPARE(d->clip(), flip);
        QCOMPARE(objectNameOf(d->gradient()), prefix + u"handle.gradient"_s);
        QCOMPARE(objectNameOf(d->data()), prefix + u"handle.data"_s);
        QCOMPARE(objectNameOfCreated(d->delegate()), prefix + u"handle.delegate"_s);
        QCOMPARE(d->border()->width(), n + 216);
        QCOMPARE(d->border()->color(), QColor(38, 60 + c, 165));
        QCOMPARE(d->shadow()->color(), QColor(38, 110 + c, 0));
        QCOMPARE(d->shadow()->opacity(), f + 0.022);
        QCOMPARE(d->shadow()->scale(), f + 1.024);
        QCOMPARE(d->shadow()->verticalOffset(), n + 217);
        QCOMPARE(d->shadow()->horizontalOffset(), n + 218);
        QCOMPARE(d->shadow()->blur(), n + 219);
        QCOMPARE(d->shadow()->visible(), flip);
        QCOMPARE(objectNameOfCreated(d->shadow()->delegate()), prefix + u"handle.shadow.delegate"_s);
        QCOMPARE(d->image()->source(), QUrl(prefix + u"handle.png"_s));
        QCOMPARE(d->image()->color(), QColor(38, 160 + c, 91));
        QCOMPARE(d->image()->fillMode(), fillModeAt(1 + stateIndex));
    }

    {
        // handle.first
        const QQStyleKitDelegateProperties *d = reader.handle()->first();
        QVERIFY(d);
        QCOMPARE(d->width(), n + 301);
        QCOMPARE(d->height(), n + 302);
        QCOMPARE(d->minimumWidth(), n + 303);
        QCOMPARE(d->minimumHeight(), n + 304);
        QCOMPARE(d->margins(), n + 305);
        QCOMPARE(d->leftMargin(), n + 306);
        QCOMPARE(d->rightMargin(), n + 307);
        QCOMPARE(d->topMargin(), n + 308);
        QCOMPARE(d->bottomMargin(), n + 309);
        QCOMPARE(d->radius(), n + 310);
        QCOMPARE(d->topLeftRadius(), n + 311);
        QCOMPARE(d->topRightRadius(), n + 312);
        QCOMPARE(d->bottomLeftRadius(), n + 313);
        QCOMPARE(d->bottomRightRadius(), n + 314);
        QCOMPARE(d->rotation(), n + 315);
        QCOMPARE(d->scale(), f + 1.033);
        QCOMPARE(d->opacity(), f + 0.031);
        QCOMPARE(d->color(), QColor(57, 10 + c, 111));
        QCOMPARE(d->alignment(), alignmentAt(2 + stateIndex));
        QCOMPARE(d->fillWidth(), flip);
        QCOMPARE(d->fillHeight(), !flip);
        QCOMPARE(d->visible(), flip);
        QCOMPARE(d->clip(), !flip);
        QCOMPARE(objectNameOf(d->gradient()), prefix + u"handle.first.gradient"_s);
        QCOMPARE(objectNameOf(d->data()), prefix + u"handle.first.data"_s);
        QCOMPARE(objectNameOfCreated(d->delegate()), prefix + u"handle.first.delegate"_s);
        QCOMPARE(d->border()->width(), n + 316);
        QCOMPARE(d->border()->color(), QColor(57, 60 + c, 202));
        QCOMPARE(d->shadow()->color(), QColor(57, 110 + c, 37));
        QCOMPARE(d->shadow()->opacity(), f + 0.032);
        QCOMPARE(d->shadow()->scale(), f + 1.034);
        QCOMPARE(d->shadow()->verticalOffset(), n + 317);
        QCOMPARE(d->shadow()->horizontalOffset(), n + 318);
        QCOMPARE(d->shadow()->blur(), n + 319);
        QCOMPARE(d->shadow()->visible(), flip);
        QCOMPARE(objectNameOfCreated(d->shadow()->delegate()), prefix + u"handle.first.shadow.delegate"_s);
        QCOMPARE(d->image()->source(), QUrl(prefix + u"handle.first.png"_s));
        QCOMPARE(d->image()->color(), QColor(57, 160 + c, 128));
        QCOMPARE(d->image()->fillMode(), fillModeAt(2 + stateIndex));
    }

    {
        // handle.second
        const QQStyleKitDelegateProperties *d = reader.handle()->second();
        QVERIFY(d);
        QCOMPARE(d->width(), n + 401);
        QCOMPARE(d->height(), n + 402);
        QCOMPARE(d->minimumWidth(), n + 403);
        QCOMPARE(d->minimumHeight(), n + 404);
        QCOMPARE(d->margins(), n + 405);
        QCOMPARE(d->leftMargin(), n + 406);
        QCOMPARE(d->rightMargin(), n + 407);
        QCOMPARE(d->topMargin(), n + 408);
        QCOMPARE(d->bottomMargin(), n + 409);
        QCOMPARE(d->radius(), n + 410);
        QCOMPARE(d->topLeftRadius(), n + 411);
        QCOMPARE(d->topRightRadius(), n + 412);
        QCOMPARE(d->bottomLeftRadius(), n + 413);
        QCOMPARE(d->bottomRightRadius(), n + 414);
        QCOMPARE(d->rotation(), n + 415);
        QCOMPARE(d->scale(), f + 1.043);
        QCOMPARE(d->opacity(), f + 0.041);
        QCOMPARE(d->color(), QColor(76, 10 + c, 148));
        QCOMPARE(d->alignment(), alignmentAt(3 + stateIndex));
        QCOMPARE(d->fillWidth(), !flip);
        QCOMPARE(d->fillHeight(), flip);
        QCOMPARE(d->visible(), !flip);
        QCOMPARE(d->clip(), flip);
        QCOMPARE(objectNameOf(d->gradient()), prefix + u"handle.second.gradient"_s);
        QCOMPARE(objectNameOf(d->data()), prefix + u"handle.second.data"_s);
        QCOMPARE(objectNameOfCreated(d->delegate()), prefix + u"handle.second.delegate"_s);
        QCOMPARE(d->border()->width(), n + 416);
        QCOMPARE(d->border()->color(), QColor(76, 60 + c, 239));
        QCOMPARE(d->shadow()->color(), QColor(76, 110 + c, 74));
        QCOMPARE(d->shadow()->opacity(), f + 0.042);
        QCOMPARE(d->shadow()->scale(), f + 1.044);
        QCOMPARE(d->shadow()->verticalOffset(), n + 417);
        QCOMPARE(d->shadow()->horizontalOffset(), n + 418);
        QCOMPARE(d->shadow()->blur(), n + 419);
        QCOMPARE(d->shadow()->visible(), !flip);
        QCOMPARE(objectNameOfCreated(d->shadow()->delegate()), prefix + u"handle.second.shadow.delegate"_s);
        QCOMPARE(d->image()->source(), QUrl(prefix + u"handle.second.png"_s));
        QCOMPARE(d->image()->color(), QColor(76, 160 + c, 165));
        QCOMPARE(d->image()->fillMode(), fillModeAt(3 + stateIndex));
    }

    {
        // indicator
        const QQStyleKitDelegateProperties *d = reader.indicator();
        QVERIFY(d);
        QCOMPARE(d->width(), n + 501);
        QCOMPARE(d->height(), n + 502);
        QCOMPARE(d->minimumWidth(), n + 503);
        QCOMPARE(d->minimumHeight(), n + 504);
        QCOMPARE(d->margins(), n + 505);
        QCOMPARE(d->leftMargin(), n + 506);
        QCOMPARE(d->rightMargin(), n + 507);
        QCOMPARE(d->topMargin(), n + 508);
        QCOMPARE(d->bottomMargin(), n + 509);
        QCOMPARE(d->radius(), n + 510);
        QCOMPARE(d->topLeftRadius(), n + 511);
        QCOMPARE(d->topRightRadius(), n + 512);
        QCOMPARE(d->bottomLeftRadius(), n + 513);
        QCOMPARE(d->bottomRightRadius(), n + 514);
        QCOMPARE(d->rotation(), n + 515);
        QCOMPARE(d->scale(), f + 1.053);
        QCOMPARE(d->opacity(), f + 0.051);
        QCOMPARE(d->color(), QColor(95, 10 + c, 185));
        QCOMPARE(d->alignment(), alignmentAt(4 + stateIndex));
        QCOMPARE(d->fillWidth(), flip);
        QCOMPARE(d->fillHeight(), !flip);
        QCOMPARE(d->visible(), !flip);
        QCOMPARE(d->clip(), flip);
        QCOMPARE(objectNameOf(d->gradient()), prefix + u"indicator.gradient"_s);
        QCOMPARE(objectNameOf(d->data()), prefix + u"indicator.data"_s);
        QCOMPARE(objectNameOfCreated(d->delegate()), prefix + u"indicator.delegate"_s);
        QCOMPARE(d->border()->width(), n + 516);
        QCOMPARE(d->border()->color(), QColor(95, 60 + c, 20));
        QCOMPARE(d->shadow()->color(), QColor(95, 110 + c, 111));
        QCOMPARE(d->shadow()->opacity(), f + 0.052);
        QCOMPARE(d->shadow()->scale(), f + 1.054);
        QCOMPARE(d->shadow()->verticalOffset(), n + 517);
        QCOMPARE(d->shadow()->horizontalOffset(), n + 518);
        QCOMPARE(d->shadow()->blur(), n + 519);
        QCOMPARE(d->shadow()->visible(), !flip);
        QCOMPARE(objectNameOfCreated(d->shadow()->delegate()), prefix + u"indicator.shadow.delegate"_s);
        QCOMPARE(d->image()->source(), QUrl(prefix + u"indicator.png"_s));
        QCOMPARE(d->image()->color(), QColor(95, 160 + c, 202));
        QCOMPARE(d->image()->fillMode(), fillModeAt(4 + stateIndex));
    }

    {
        // indicator.foreground
        const QQStyleKitDelegateProperties *d = reader.indicator()->foreground();
        QVERIFY(d);
        QCOMPARE(d->width(), n + 601);
        QCOMPARE(d->height(), n + 602);
        QCOMPARE(d->minimumWidth(), n + 603);
        QCOMPARE(d->minimumHeight(), n + 604);
        QCOMPARE(d->margins(), n + 605);
        QCOMPARE(d->leftMargin(), n + 606);
        QCOMPARE(d->rightMargin(), n + 607);
        QCOMPARE(d->topMargin(), n + 608);
        QCOMPARE(d->bottomMargin(), n + 609);
        QCOMPARE(d->radius(), n + 610);
        QCOMPARE(d->topLeftRadius(), n + 611);
        QCOMPARE(d->topRightRadius(), n + 612);
        QCOMPARE(d->bottomLeftRadius(), n + 613);
        QCOMPARE(d->bottomRightRadius(), n + 614);
        QCOMPARE(d->rotation(), n + 615);
        QCOMPARE(d->scale(), f + 1.063);
        QCOMPARE(d->opacity(), f + 0.061);
        QCOMPARE(d->color(), QColor(114, 10 + c, 222));
        QCOMPARE(d->alignment(), alignmentAt(5 + stateIndex));
        QCOMPARE(d->fillWidth(), !flip);
        QCOMPARE(d->fillHeight(), flip);
        QCOMPARE(d->visible(), flip);
        QCOMPARE(d->clip(), !flip);
        QCOMPARE(objectNameOf(d->gradient()), prefix + u"indicator.foreground.gradient"_s);
        QCOMPARE(objectNameOf(d->data()), prefix + u"indicator.foreground.data"_s);
        QCOMPARE(objectNameOfCreated(d->delegate()), prefix + u"indicator.foreground.delegate"_s);
        QCOMPARE(d->border()->width(), n + 616);
        QCOMPARE(d->border()->color(), QColor(114, 60 + c, 57));
        QCOMPARE(d->shadow()->color(), QColor(114, 110 + c, 148));
        QCOMPARE(d->shadow()->opacity(), f + 0.062);
        QCOMPARE(d->shadow()->scale(), f + 1.064);
        QCOMPARE(d->shadow()->verticalOffset(), n + 617);
        QCOMPARE(d->shadow()->horizontalOffset(), n + 618);
        QCOMPARE(d->shadow()->blur(), n + 619);
        QCOMPARE(d->shadow()->visible(), flip);
        QCOMPARE(objectNameOfCreated(d->shadow()->delegate()), prefix + u"indicator.foreground.shadow.delegate"_s);
        QCOMPARE(d->image()->source(), QUrl(prefix + u"indicator.foreground.png"_s));
        QCOMPARE(d->image()->color(), QColor(114, 160 + c, 239));
        QCOMPARE(d->image()->fillMode(), fillModeAt(5 + stateIndex));
    }

    {
        // indicator.first
        const QQStyleKitDelegateProperties *d = reader.indicator()->first();
        QVERIFY(d);
        QCOMPARE(d->width(), n + 701);
        QCOMPARE(d->height(), n + 702);
        QCOMPARE(d->minimumWidth(), n + 703);
        QCOMPARE(d->minimumHeight(), n + 704);
        QCOMPARE(d->margins(), n + 705);
        QCOMPARE(d->leftMargin(), n + 706);
        QCOMPARE(d->rightMargin(), n + 707);
        QCOMPARE(d->topMargin(), n + 708);
        QCOMPARE(d->bottomMargin(), n + 709);
        QCOMPARE(d->radius(), n + 710);
        QCOMPARE(d->topLeftRadius(), n + 711);
        QCOMPARE(d->topRightRadius(), n + 712);
        QCOMPARE(d->bottomLeftRadius(), n + 713);
        QCOMPARE(d->bottomRightRadius(), n + 714);
        QCOMPARE(d->rotation(), n + 715);
        QCOMPARE(d->scale(), f + 1.073);
        QCOMPARE(d->opacity(), f + 0.071);
        QCOMPARE(d->color(), QColor(133, 10 + c, 3));
        QCOMPARE(d->alignment(), alignmentAt(6 + stateIndex));
        QCOMPARE(d->fillWidth(), flip);
        QCOMPARE(d->fillHeight(), !flip);
        QCOMPARE(d->visible(), !flip);
        QCOMPARE(d->clip(), flip);
        QCOMPARE(objectNameOf(d->gradient()), prefix + u"indicator.first.gradient"_s);
        QCOMPARE(objectNameOf(d->data()), prefix + u"indicator.first.data"_s);
        QCOMPARE(objectNameOfCreated(d->delegate()), prefix + u"indicator.first.delegate"_s);
        QCOMPARE(d->border()->width(), n + 716);
        QCOMPARE(d->border()->color(), QColor(133, 60 + c, 94));
        QCOMPARE(d->shadow()->color(), QColor(133, 110 + c, 185));
        QCOMPARE(d->shadow()->opacity(), f + 0.072);
        QCOMPARE(d->shadow()->scale(), f + 1.074);
        QCOMPARE(d->shadow()->verticalOffset(), n + 717);
        QCOMPARE(d->shadow()->horizontalOffset(), n + 718);
        QCOMPARE(d->shadow()->blur(), n + 719);
        QCOMPARE(d->shadow()->visible(), flip);
        QCOMPARE(objectNameOfCreated(d->shadow()->delegate()), prefix + u"indicator.first.shadow.delegate"_s);
        QCOMPARE(d->image()->source(), QUrl(prefix + u"indicator.first.png"_s));
        QCOMPARE(d->image()->color(), QColor(133, 160 + c, 20));
        QCOMPARE(d->image()->fillMode(), fillModeAt(6 + stateIndex));
    }

    {
        // indicator.first.foreground
        const QQStyleKitDelegateProperties *d = reader.indicator()->first()->foreground();
        QVERIFY(d);
        QCOMPARE(d->width(), n + 801);
        QCOMPARE(d->height(), n + 802);
        QCOMPARE(d->minimumWidth(), n + 803);
        QCOMPARE(d->minimumHeight(), n + 804);
        QCOMPARE(d->margins(), n + 805);
        QCOMPARE(d->leftMargin(), n + 806);
        QCOMPARE(d->rightMargin(), n + 807);
        QCOMPARE(d->topMargin(), n + 808);
        QCOMPARE(d->bottomMargin(), n + 809);
        QCOMPARE(d->radius(), n + 810);
        QCOMPARE(d->topLeftRadius(), n + 811);
        QCOMPARE(d->topRightRadius(), n + 812);
        QCOMPARE(d->bottomLeftRadius(), n + 813);
        QCOMPARE(d->bottomRightRadius(), n + 814);
        QCOMPARE(d->rotation(), n + 815);
        QCOMPARE(d->scale(), f + 1.083);
        QCOMPARE(d->opacity(), f + 0.081);
        QCOMPARE(d->color(), QColor(152, 10 + c, 40));
        QCOMPARE(d->alignment(), alignmentAt(7 + stateIndex));
        QCOMPARE(d->fillWidth(), !flip);
        QCOMPARE(d->fillHeight(), flip);
        QCOMPARE(d->visible(), !flip);
        QCOMPARE(d->clip(), flip);
        QCOMPARE(objectNameOf(d->gradient()), prefix + u"indicator.first.foreground.gradient"_s);
        QCOMPARE(objectNameOf(d->data()), prefix + u"indicator.first.foreground.data"_s);
        QCOMPARE(objectNameOfCreated(d->delegate()), prefix + u"indicator.first.foreground.delegate"_s);
        QCOMPARE(d->border()->width(), n + 816);
        QCOMPARE(d->border()->color(), QColor(152, 60 + c, 131));
        QCOMPARE(d->shadow()->color(), QColor(152, 110 + c, 222));
        QCOMPARE(d->shadow()->opacity(), f + 0.082);
        QCOMPARE(d->shadow()->scale(), f + 1.084);
        QCOMPARE(d->shadow()->verticalOffset(), n + 817);
        QCOMPARE(d->shadow()->horizontalOffset(), n + 818);
        QCOMPARE(d->shadow()->blur(), n + 819);
        QCOMPARE(d->shadow()->visible(), !flip);
        QCOMPARE(objectNameOfCreated(d->shadow()->delegate()), prefix + u"indicator.first.foreground.shadow.delegate"_s);
        QCOMPARE(d->image()->source(), QUrl(prefix + u"indicator.first.foreground.png"_s));
        QCOMPARE(d->image()->color(), QColor(152, 160 + c, 57));
        QCOMPARE(d->image()->fillMode(), fillModeAt(7 + stateIndex));
    }

    {
        // indicator.second
        const QQStyleKitDelegateProperties *d = reader.indicator()->second();
        QVERIFY(d);
        QCOMPARE(d->width(), n + 901);
        QCOMPARE(d->height(), n + 902);
        QCOMPARE(d->minimumWidth(), n + 903);
        QCOMPARE(d->minimumHeight(), n + 904);
        QCOMPARE(d->margins(), n + 905);
        QCOMPARE(d->leftMargin(), n + 906);
        QCOMPARE(d->rightMargin(), n + 907);
        QCOMPARE(d->topMargin(), n + 908);
        QCOMPARE(d->bottomMargin(), n + 909);
        QCOMPARE(d->radius(), n + 910);
        QCOMPARE(d->topLeftRadius(), n + 911);
        QCOMPARE(d->topRightRadius(), n + 912);
        QCOMPARE(d->bottomLeftRadius(), n + 913);
        QCOMPARE(d->bottomRightRadius(), n + 914);
        QCOMPARE(d->rotation(), n + 915);
        QCOMPARE(d->scale(), f + 1.093);
        QCOMPARE(d->opacity(), f + 0.091);
        QCOMPARE(d->color(), QColor(171, 10 + c, 77));
        QCOMPARE(d->alignment(), alignmentAt(8 + stateIndex));
        QCOMPARE(d->fillWidth(), flip);
        QCOMPARE(d->fillHeight(), !flip);
        QCOMPARE(d->visible(), flip);
        QCOMPARE(d->clip(), !flip);
        QCOMPARE(objectNameOf(d->gradient()), prefix + u"indicator.second.gradient"_s);
        QCOMPARE(objectNameOf(d->data()), prefix + u"indicator.second.data"_s);
        QCOMPARE(objectNameOfCreated(d->delegate()), prefix + u"indicator.second.delegate"_s);
        QCOMPARE(d->border()->width(), n + 916);
        QCOMPARE(d->border()->color(), QColor(171, 60 + c, 168));
        QCOMPARE(d->shadow()->color(), QColor(171, 110 + c, 3));
        QCOMPARE(d->shadow()->opacity(), f + 0.092);
        QCOMPARE(d->shadow()->scale(), f + 1.094);
        QCOMPARE(d->shadow()->verticalOffset(), n + 917);
        QCOMPARE(d->shadow()->horizontalOffset(), n + 918);
        QCOMPARE(d->shadow()->blur(), n + 919);
        QCOMPARE(d->shadow()->visible(), !flip);
        QCOMPARE(objectNameOfCreated(d->shadow()->delegate()), prefix + u"indicator.second.shadow.delegate"_s);
        QCOMPARE(d->image()->source(), QUrl(prefix + u"indicator.second.png"_s));
        QCOMPARE(d->image()->color(), QColor(171, 160 + c, 94));
        QCOMPARE(d->image()->fillMode(), fillModeAt(8 + stateIndex));
    }

    {
        // indicator.second.foreground
        const QQStyleKitDelegateProperties *d = reader.indicator()->second()->foreground();
        QVERIFY(d);
        QCOMPARE(d->width(), n + 1001);
        QCOMPARE(d->height(), n + 1002);
        QCOMPARE(d->minimumWidth(), n + 1003);
        QCOMPARE(d->minimumHeight(), n + 1004);
        QCOMPARE(d->margins(), n + 1005);
        QCOMPARE(d->leftMargin(), n + 1006);
        QCOMPARE(d->rightMargin(), n + 1007);
        QCOMPARE(d->topMargin(), n + 1008);
        QCOMPARE(d->bottomMargin(), n + 1009);
        QCOMPARE(d->radius(), n + 1010);
        QCOMPARE(d->topLeftRadius(), n + 1011);
        QCOMPARE(d->topRightRadius(), n + 1012);
        QCOMPARE(d->bottomLeftRadius(), n + 1013);
        QCOMPARE(d->bottomRightRadius(), n + 1014);
        QCOMPARE(d->rotation(), n + 1015);
        QCOMPARE(d->scale(), f + 1.103);
        QCOMPARE(d->opacity(), f + 0.101);
        QCOMPARE(d->color(), QColor(190, 10 + c, 114));
        QCOMPARE(d->alignment(), alignmentAt(9 + stateIndex));
        QCOMPARE(d->fillWidth(), !flip);
        QCOMPARE(d->fillHeight(), flip);
        QCOMPARE(d->visible(), !flip);
        QCOMPARE(d->clip(), flip);
        QCOMPARE(objectNameOf(d->gradient()), prefix + u"indicator.second.foreground.gradient"_s);
        QCOMPARE(objectNameOf(d->data()), prefix + u"indicator.second.foreground.data"_s);
        QCOMPARE(objectNameOfCreated(d->delegate()), prefix + u"indicator.second.foreground.delegate"_s);
        QCOMPARE(d->border()->width(), n + 1016);
        QCOMPARE(d->border()->color(), QColor(190, 60 + c, 205));
        QCOMPARE(d->shadow()->color(), QColor(190, 110 + c, 40));
        QCOMPARE(d->shadow()->opacity(), f + 0.102);
        QCOMPARE(d->shadow()->scale(), f + 1.104);
        QCOMPARE(d->shadow()->verticalOffset(), n + 1017);
        QCOMPARE(d->shadow()->horizontalOffset(), n + 1018);
        QCOMPARE(d->shadow()->blur(), n + 1019);
        QCOMPARE(d->shadow()->visible(), flip);
        QCOMPARE(objectNameOfCreated(d->shadow()->delegate()), prefix + u"indicator.second.foreground.shadow.delegate"_s);
        QCOMPARE(d->image()->source(), QUrl(prefix + u"indicator.second.foreground.png"_s));
        QCOMPARE(d->image()->color(), QColor(190, 160 + c, 131));
        QCOMPARE(d->image()->fillMode(), fillModeAt(9 + stateIndex));
    }

    {
        // text
        const QQStyleKitTextProperties *t = reader.text();
        QVERIFY(t);
        QCOMPARE(t->color(), QColor(209, 10 + c, 151));
        QCOMPARE(t->alignment(), alignmentAt(10 + stateIndex));
        QCOMPARE(t->bold(), flip);
        QCOMPARE(t->italic(), !flip);
        QCOMPARE(t->pointSize(), n + 1101);
        QCOMPARE(t->padding(), n + 1102);
        QCOMPARE(t->leftPadding(), n + 1103);
        QCOMPARE(t->rightPadding(), n + 1104);
        QCOMPARE(t->topPadding(), n + 1105);
        QCOMPARE(t->bottomPadding(), n + 1106);
    }

    {
        // text.first
        const QQStyleKitTextProperties *t = reader.text()->first();
        QVERIFY(t);
        QCOMPARE(t->color(), QColor(228, 10 + c, 188));
        QCOMPARE(t->alignment(), alignmentAt(11 + stateIndex));
        QCOMPARE(t->bold(), !flip);
        QCOMPARE(t->italic(), flip);
        QCOMPARE(t->pointSize(), n + 1201);
        QCOMPARE(t->padding(), n + 1202);
        QCOMPARE(t->leftPadding(), n + 1203);
        QCOMPARE(t->rightPadding(), n + 1204);
        QCOMPARE(t->topPadding(), n + 1205);
        QCOMPARE(t->bottomPadding(), n + 1206);
    }

    {
        // text.second
        const QQStyleKitTextProperties *t = reader.text()->second();
        QVERIFY(t);
        QCOMPARE(t->color(), QColor(247, 10 + c, 225));
        QCOMPARE(t->alignment(), alignmentAt(12 + stateIndex));
        QCOMPARE(t->bold(), flip);
        QCOMPARE(t->italic(), !flip);
        QCOMPARE(t->pointSize(), n + 1301);
        QCOMPARE(t->padding(), n + 1302);
        QCOMPARE(t->leftPadding(), n + 1303);
        QCOMPARE(t->rightPadding(), n + 1304);
        QCOMPARE(t->topPadding(), n + 1305);
        QCOMPARE(t->bottomPadding(), n + 1306);
    }
}

QTEST_MAIN(tst_StyleKit)

#include "tst_stylekit.moc"
