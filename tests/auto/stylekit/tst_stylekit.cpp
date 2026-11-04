// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>
#include <QtCore/qcoreapplication.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcomponent.h>

#include <QtLabsStyleKit/private/qqstylekit_p.h>
#include <QtLabsStyleKit/private/qqstylekitstyle_p.h>
#include <QtLabsStyleKit/private/qqstylekitreader_p.h>

#define LOAD_STYLE(fileName) \
    QQmlEngine engine; \
    QQmlComponent component(&engine, QUrl::fromLocalFile( \
            QStringLiteral(QT_QMLTEST_DATADIR) \
            + QLatin1Char('/') + QStringLiteral(#fileName))); \
    QVERIFY2(component.isReady(), qPrintable(component.errorString())); \
    QScopedPointer<QQStyleKitStyle> style(qobject_cast<QQStyleKitStyle *>(component.create())); \
    QVERIFY(style);

class tst_StyleKit : public QObject
{
    Q_OBJECT

private slots:
    void propagationStyleAndFallback();
    void propagationThemeAndStyle();
    void propagationThemeStyleFallbackStyle_data();
    void propagationThemeStyleFallbackStyle();
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

QTEST_MAIN(tst_StyleKit)

#include "tst_stylekit.moc"
