// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/QSignalSpy>

#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlengine.h>

#include <QtQuick/private/qquickpalette_p.h>
#include <QtQuick/private/qquickabstractpaletteprovider_p.h>
#include <QtQuick/private/qquickpalettecolorprovider_p.h>

#include <QtQuickTestUtils/private/qmlutils_p.h>

// Use this to get a more descriptive failure message (QTBUG-87039).
#define COMPARE_PALETTES(actualPalette, expectedPalette) \
    QVERIFY2(actualPalette == expectedPalette, \
        qPrintable(QString::fromLatin1("\n   Actual:    %1\n   Expected:  %2") \
            .arg(QDebug::toString(actualPalette)).arg(QDebug::toString(expectedPalette))));

class tst_QQuickPalette : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQuickPalette();

private Q_SLOTS:
    void resolvingColor();
    void resolvingColor_data();

    void newColorSubgroup();
    void newColorSubgroup_data();

    void onlyRespectiveColorSubgroupChangedAfterAssigment();

    void paletteChangedWhenColorGroupChanged();

    void createDefault();

    void changeCurrentColorGroup();

    void inheritColor();

    void inheritCurrentColor();

    void overrideColor();

    void resolveColor();

    void createFromQtPalette();
    void convertToQtPalette();

    void qml_data();
    void qml();
};

using GroupGetter = QQuickColorGroup* (QQuickPalette::* )() const;
Q_DECLARE_METATYPE(GroupGetter);

tst_QQuickPalette::tst_QQuickPalette()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_QQuickPalette::resolvingColor()
{
    QFETCH(QPalette::ColorGroup, colorGroup);
    QFETCH(GroupGetter, getter);

    QQuickPalette p;
    p.setWindowText(Qt::red);

    auto g = (p.*getter)();

    QVERIFY(g);

    g->setWindowText(Qt::green);
    p.setCurrentGroup(colorGroup);

    QCOMPARE(p.windowText(), Qt::green);
}

void tst_QQuickPalette::resolvingColor_data()
{
    QTest::addColumn<QPalette::ColorGroup>("colorGroup");
    QTest::addColumn<GroupGetter>("getter");

    QTest::addRow("Inactive") << QPalette::Inactive << &QQuickPalette::inactive;
    QTest::addRow("Disabled") << QPalette::Disabled << &QQuickPalette::disabled;
}

using GroupSetter   = void (QQuickPalette::* )(QQuickColorGroup *);
Q_DECLARE_METATYPE(GroupSetter);

using GroupNotifier = void (QQuickPalette::* )();
Q_DECLARE_METATYPE(GroupNotifier);

void tst_QQuickPalette::newColorSubgroup()
{
    QFETCH(GroupGetter, getter);
    QFETCH(GroupSetter, setter);
    QFETCH(GroupNotifier, notifier);

    {
        QQuickPalette p;
        p.fromQPalette(Qt::blue);

        auto defaultGroup = (p.*getter)();
        QVERIFY(defaultGroup);

        QSignalSpy subgroupChanged(&p, notifier);
        QSignalSpy paletteChanged(&p, &QQuickPalette::changed);

        QQuickPalette anotherPalette;
        anotherPalette.fromQPalette(Qt::red);
        (p.*setter)((anotherPalette.*getter)());

        QCOMPARE(subgroupChanged.size(), 1);
        QCOMPARE(paletteChanged.size(), 1);
    }
}

void tst_QQuickPalette::newColorSubgroup_data()
{
    QTest::addColumn<GroupGetter>("getter");
    QTest::addColumn<GroupSetter>("setter");
    QTest::addColumn<GroupNotifier>("notifier");

    QTest::addRow("Active")   << &QQuickPalette::active   << &QQuickPalette::setActive
                              << &QQuickPalette::activeChanged;
    QTest::addRow("Inactive") << &QQuickPalette::inactive << &QQuickPalette::setInactive
                              << &QQuickPalette::inactiveChanged;
    QTest::addRow("Disabled") << &QQuickPalette::disabled << &QQuickPalette::setDisabled
                              << &QQuickPalette::disabledChanged;
}

void tst_QQuickPalette::onlyRespectiveColorSubgroupChangedAfterAssigment()
{
    QQuickPalette palette;
    palette.setWindow(Qt::red);

    QQuickPalette anotherPalette;
    anotherPalette.active()->setWindow(Qt::green);

    // Only active subgroup should be copied
    palette.setActive(anotherPalette.active());

    QCOMPARE(palette.active()->window(), Qt::green);
    QCOMPARE(palette.disabled()->window(), Qt::red);
    QCOMPARE(palette.inactive()->window(), Qt::red);
}

void tst_QQuickPalette::paletteChangedWhenColorGroupChanged()
{
    QQuickPalette p;
    QSignalSpy sp(&p, &QQuickPalette::changed);

    p.active()->setMid(Qt::red);
    p.inactive()->setMid(Qt::green);
    p.disabled()->setMid(Qt::blue);

    QCOMPARE(sp.size(), 3);
}

void tst_QQuickPalette::createDefault()
{
    QQuickPalette palette;

    QCOMPARE(palette.currentColorGroup(), QPalette::Active);
    QCOMPARE(palette.active()->groupTag(), QPalette::Active);
    QCOMPARE(palette.inactive()->groupTag(), QPalette::Inactive);
    QCOMPARE(palette.disabled()->groupTag(), QPalette::Disabled);
}

void tst_QQuickPalette::changeCurrentColorGroup()
{
    QQuickPalette palette;

    QSignalSpy ss(&palette, &QQuickPalette::changed);
    palette.setCurrentGroup(QPalette::Disabled);

    QCOMPARE(palette.currentColorGroup(), QPalette::Disabled);
    QCOMPARE(ss.size(), 1);
}

void tst_QQuickPalette::inheritColor()
{
    QQuickPalette parentPalette;
    parentPalette.setWindowText(Qt::red);

    QQuickPalette quickPalette;
    quickPalette.inheritPalette(parentPalette.toQPalette());

    QCOMPARE(quickPalette.windowText(), Qt::red);

    QQuickPalette childQuickPalette;
    childQuickPalette.inheritPalette(quickPalette.toQPalette());

    QCOMPARE(childQuickPalette.windowText(), Qt::red);
}

void tst_QQuickPalette::inheritCurrentColor()
{
    QQuickPalette parentPalette;
    parentPalette.setWindowText(Qt::green);
    parentPalette.disabled()->setWindowText(Qt::red);


    QQuickPalette quickPalette;
    quickPalette.inheritPalette(parentPalette.toQPalette());
    quickPalette.setCurrentGroup(QPalette::Disabled);

    QCOMPARE(quickPalette.windowText(), Qt::red);
}

void tst_QQuickPalette::overrideColor()
{
    QQuickPalette rootPalette;
    rootPalette.setWindowText(Qt::red);

    QQuickPalette palette;
    palette.inheritPalette(rootPalette.toQPalette());
    palette.setWindowText(Qt::yellow);

    QCOMPARE(palette.windowText(), Qt::yellow);

    QQuickPalette childPalette;
    childPalette.inheritPalette(palette.toQPalette());
    childPalette.disabled()->setWindowText(Qt::green);

    // Color is not set for the current group. Use parent color
    QCOMPARE(childPalette.windowText(), Qt::yellow);

    // Change current group to use color, specified for this particular group
    childPalette.setCurrentGroup(QPalette::Disabled);

    QCOMPARE(childPalette.windowText(), Qt::green);
}

void tst_QQuickPalette::resolveColor()
{
    QQuickPalette palette;
    palette.setWindowText(Qt::red);

    // Disabled color should be red, because disabled palette is not specified
    palette.setCurrentGroup(QPalette::Disabled);
    QCOMPARE(palette.windowText(), Qt::red);

    // Color is changed for disabled palette, because current color group is QPalette::Disabled
    palette.disabled()->setWindowText(Qt::yellow);
    QCOMPARE(palette.windowText(), Qt::yellow);
    QCOMPARE(palette.disabled()->windowText(), Qt::yellow);

    // Change color group back to active
    palette.setCurrentGroup(QPalette::Active);
    QCOMPARE(palette.windowText(), Qt::red);
}

void tst_QQuickPalette::createFromQtPalette()
{
    QQuickPalette palette;
    QPalette somePalette(Qt::red);

    QSignalSpy sp(&palette, &QQuickColorGroup::changed);

    palette.fromQPalette(QPalette());
    QCOMPARE(sp.size(), 0);

    palette.fromQPalette(somePalette);
    QCOMPARE(sp.size(), 1);
}

void tst_QQuickPalette::convertToQtPalette()
{
    QQuickPalette palette;

    QPalette somePalette(Qt::red);
    palette.fromQPalette(somePalette);

    auto pp = palette.paletteProvider();
    QVERIFY(pp);

    QCOMPARE(palette.toQPalette(), somePalette.resolve(pp->defaultPalette()));
}

// QTBUG-93691
void tst_QQuickPalette::qml_data()
{
    QTest::addColumn<QString>("testFile");
    QTest::addColumn<QPalette>("expectedPalette");

    const QPalette defaultPalette;
    QTest::newRow("Item") << "palette-item-default.qml" << defaultPalette;
    QTest::newRow("Window") << "palette-window-default.qml" << defaultPalette;

    QPalette customPalette;
    customPalette.setColor(QPalette::AlternateBase, QColor("aqua"));
    customPalette.setColor(QPalette::Base, QColor("azure"));
    customPalette.setColor(QPalette::BrightText, QColor("beige"));
    customPalette.setColor(QPalette::Button, QColor("bisque"));
    customPalette.setColor(QPalette::ButtonText, QColor("chocolate"));
    customPalette.setColor(QPalette::Dark, QColor("coral"));
    customPalette.setColor(QPalette::Highlight, QColor("crimson"));
    customPalette.setColor(QPalette::HighlightedText, QColor("fuchsia"));
    customPalette.setColor(QPalette::Light, QColor("gold"));
    customPalette.setColor(QPalette::Link, QColor("indigo"));
    customPalette.setColor(QPalette::LinkVisited, QColor("ivory"));
    customPalette.setColor(QPalette::Mid, QColor("khaki"));
    customPalette.setColor(QPalette::Midlight, QColor("lavender"));
    customPalette.setColor(QPalette::Shadow, QColor("linen"));
    customPalette.setColor(QPalette::Text, QColor("moccasin"));
    customPalette.setColor(QPalette::ToolTipBase, QColor("navy"));
    customPalette.setColor(QPalette::ToolTipText, QColor("orchid"));
    customPalette.setColor(QPalette::Window, QColor("plum"));
    customPalette.setColor(QPalette::WindowText, QColor("salmon"));

    QTest::newRow("Item:custom") << "palette-item-custom.qml" << customPalette;
    QTest::newRow("Window:custom") << "palette-window-custom.qml" << customPalette;
}

void tst_QQuickPalette::qml()
{
    QFETCH(QString, testFile);
    QFETCH(QPalette, expectedPalette);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.loadUrl(testFileUrl(testFile));

    QScopedPointer<QObject> object(component.create());
    QVERIFY2(!object.isNull(), qPrintable(component.errorString()));

    const QVariant var = object->property("palette");
    QVERIFY(var.isValid());
    COMPARE_PALETTES(var.value<QQuickPalette*>()->toQPalette(), expectedPalette);
}

QTEST_MAIN(tst_QQuickPalette)

#include "tst_qquickpalette.moc"
