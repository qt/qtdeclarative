// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtGui/qpa/qplatformtheme.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtQml/qqmlcomponent.h>
#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>
#include <QtQuick/qquickview.h>
#include <QtQuick/private/qquicktext_p_p.h>
#include <QtQuickTest/quicktest.h>
#include <QtQuickTemplates2/private/qquickbutton_p.h>
#include <QtQuickTemplates2/private/qquickaction_p.h>
#include <QtQuickControls2Impl/private/qquickmnemoniclabel_p.h>
#include <QtQuickControlsTestUtils/private/qtest_quickcontrols_p.h>

#include <memory>

#ifndef QTEST_THROW_ON_FAIL
# error This test requires QTEST_THROW_ON_FAIL being active.
#endif
#ifndef QTEST_THROW_ON_SKIP
# error This test requires QTEST_THROW_ON_SKIP being active.
#endif

class tst_QQuickAbstractButton : public QObject
{
    Q_OBJECT

public:
    tst_QQuickAbstractButton();

private slots:
    void initTestCase();
    void cleanup();
    void cleanupTestCase();

    void mnemonics_data();
    void mnemonics();

private:
    void ensureViewVisible();

    bool platformSupportsMnemonics = false;
    bool platformUnderlinesShortcuts = false;
    // Reused for every row of mnemonics() (within a given style;
    // initTestCase is called once for each style that
    // QTEST_QUICKCONTROLS_MAIN runs the test with) so that we don't pay
    // the cost of creating a new window (and QML engine) per data row.
    // Use it for future tests that have a lot of rows, where possible.
    std::unique_ptr<QQuickView> view;
};

tst_QQuickAbstractButton::tst_QQuickAbstractButton()
{
    platformSupportsMnemonics = QGuiApplicationPrivate::platformTheme()->themeHint(
        QPlatformTheme::MnemonicsEnabled).toBool();
    platformUnderlinesShortcuts = QGuiApplicationPrivate::platformTheme()->themeHint(
        QPlatformTheme::UnderlineShortcut).toBool();
}

void tst_QQuickAbstractButton::initTestCase()
{
    view = std::make_unique<QQuickView>();
    view->setResizeMode(QQuickView::SizeRootObjectToView);
    // Account for minimum window size on Windows, otherwise we get warnings.
    // This and the resize mode above also just happen to shave off 3.5
    // seconds on Ubuntu due to not resizing for each row.
    view->resize(100, 100);
}

void tst_QQuickAbstractButton::ensureViewVisible()
{
    if (view->isVisible())
        return;

    view->show();
    QVERIFY(QTest::qWaitForWindowExposed(view.get()));
}

void tst_QQuickAbstractButton::cleanup()
{
    view->setContent({}, nullptr, nullptr);
}

void tst_QQuickAbstractButton::cleanupTestCase()
{
    view.reset();
}

void tst_QQuickAbstractButton::mnemonics_data()
{
    QTest::addColumn<bool>("expectedEnabledByDefault");

    // The data tag is used as the QML type to instantiate; see mnemonics().
    QTest::newRow("Button") << platformSupportsMnemonics;
    QTest::newRow("CheckBox") << platformSupportsMnemonics;
    QTest::newRow("MenuItem") << platformSupportsMnemonics;
    QTest::newRow("MenuBarItem") << platformSupportsMnemonics;
    QTest::newRow("RadioButton") << platformSupportsMnemonics;
    QTest::newRow("Switch") << platformSupportsMnemonics;
    QTest::newRow("ToolButton") << platformSupportsMnemonics;
    // These types explicitly disable mnemonics regardless of platform.
    QTest::newRow("ItemDelegate") << false;
    QTest::newRow("CheckDelegate") << false;
    QTest::newRow("RadioDelegate") << false;
    QTest::newRow("SwipeDelegate") << false;
    QTest::newRow("SwitchDelegate") << false;
    QTest::newRow("DelayButton") << false;
}

void tst_QQuickAbstractButton::mnemonics()
{
    QFETCH(bool, expectedEnabledByDefault);
    const QString qmlType = QString::fromUtf8(QTest::currentDataTag());

    ensureViewVisible();
    QVERIFY(QTest::qWaitForWindowActive(view.get()));

    QQmlComponent component(view->engine());
    component.setData(QStringLiteral(
        "import QtQuick.Controls\n"
        "%1 {\n"
        "    text: \"M&nemonic\"\n"
        "    action: useAction ? mnemonicAction : null\n"
        "    property bool useAction\n"
        "    Action {\n"
        "        id: mnemonicAction\n"
        "        text: \"&Action\"\n"
        "    }\n"
        "}\n").arg(qmlType).toUtf8(), QUrl());
    QVERIFY2(component.isReady(), qPrintable(component.errorString()));
    QObject *object = component.create();
    QVERIFY2(object, qPrintable(component.errorString()));
    view->setContent(QUrl(), &component, object);
    QQuickItem *root = view->rootObject();
    QVERIFY(root);

    auto *control = qobject_cast<QQuickAbstractButton *>(view->rootObject());
    QVERIFY(control);
    // The window itself was already shown/exposed/activated above; we just need to make
    // sure this row's freshly created item has settled before we inspect it.
    QVERIFY(QQuickTest::qWaitForPolish(control));
    auto *textItem = qobject_cast<QQuickText *>(control->findChild<QQuickText *>());
    QVERIFY(textItem);
    auto *textItemPrivate = QQuickTextPrivate::get(textItem);

    // First, check the defaults.
    const bool usesMnemonicLabel = qobject_cast<QQuickMnemonicLabel *>(textItem);
    const bool mnemonicStripped = usesMnemonicLabel && expectedEnabledByDefault;
    // Whether the mnemonic character is actually underlined additionally depends on
    // QPlatformTheme::UnderlineShortcut (which is false for e.g. macOS).
    const bool mnemonicUnderlined = mnemonicStripped && platformUnderlinesShortcuts;
    // The "&" should always be stripped from the displayed text when mnemonics are enabled,
    // even if the platform doesn't underline the shortcut.
    QCOMPARE(textItemPrivate->text, mnemonicStripped ? "Mnemonic" : "M&nemonic");
    // There should only be one QTextLayout::FormatRange applied to the text: the one for the
    // underline.
    QCOMPARE(textItemPrivate->layout.formats().size(), mnemonicUnderlined ? 1 : 0);
    if (mnemonicUnderlined) {
        const QTextLayout::FormatRange underlineFormatRange
            = textItemPrivate->layout.formats().constFirst();
        QCOMPARE(underlineFormatRange.start, 1);
        QCOMPARE(underlineFormatRange.length, 1);
        QVERIFY(underlineFormatRange.format.fontUnderline());
    }

    // Set the text to something else to ensure that it picks up changes.
    control->setText(QStringLiteral("&Hello"));
    QCOMPARE(control->text(), QStringLiteral("&Hello"));
    QCOMPARE(textItemPrivate->text, mnemonicStripped ? "Hello" : "&Hello");

    const QSignalSpy clickSpy(control, SIGNAL(clicked()));
    QVERIFY(clickSpy.isValid());
    int expectedClickedCount = 0;

    // Pressing the shortcut while visible should click the button.
    if (expectedEnabledByDefault)
        ++expectedClickedCount;
    QTest::keyClick(view.get(), Qt::Key_H, Qt::AltModifier);
    QCOMPARE(clickSpy.count(), expectedClickedCount);

    // Pressing the shortcut while hidden shouldn't click the button.
    control->setVisible(false);
    QTest::keyClick(view.get(), Qt::Key_H, Qt::AltModifier);
    QCOMPARE(clickSpy.count(), expectedClickedCount);

    // Pressing the shortcut after showing should click the button.
    if (expectedEnabledByDefault)
        ++expectedClickedCount;
    control->setVisible(true);
    QTest::keyClick(view.get(), Qt::Key_H, Qt::AltModifier);
    QCOMPARE(clickSpy.count(), expectedClickedCount);

    // Change the shortcut by changing the mnemonic.
    control->setText(QStringLiteral("Te&st"));
    QCOMPARE(control->text(), QStringLiteral("Te&st"));

    // The old shortcut should no longer work.
    QTest::keyClick(view.get(), Qt::Key_H, Qt::AltModifier);
    QCOMPARE(clickSpy.count(), expectedClickedCount);

    // The new shortcut should.
    if (expectedEnabledByDefault)
        ++expectedClickedCount;
    QTest::keyClick(view.get(), Qt::Key_S, Qt::AltModifier);
    QCOMPARE(clickSpy.count(), expectedClickedCount);

    // Hide the button and change the shortcut back to the original one; shouldn't click.
    control->setVisible(false);
    control->setText(QStringLiteral("&Hidden"));
    QTest::keyClick(view.get(), Qt::Key_H, Qt::AltModifier);
    QCOMPARE(clickSpy.count(), expectedClickedCount);

    // Restore visibility; should click.
    if (expectedEnabledByDefault)
        ++expectedClickedCount;
    control->setVisible(true);
    QTest::keyClick(view.get(), Qt::Key_H, Qt::AltModifier);
    QCOMPARE(clickSpy.count(), expectedClickedCount);

    // Reset the text and start using the action.
    control->resetText();
    control->setProperty("useAction", true);
    QQuickAction *action = control->action();
    QVERIFY(action);
    QCOMPARE(action->text(), QStringLiteral("&Action"));

    QSignalSpy actionSpy(action, SIGNAL(triggered(QObject*)));
    QVERIFY(actionSpy.isValid());

    // Mnemonics in Action text should behave the same as the button's text.
    int expectedActionTriggeredCount = 0;
    if (expectedEnabledByDefault) {
        ++expectedClickedCount;
        ++expectedActionTriggeredCount;
    }
    QTest::keyClick(view.get(), Qt::Key_A, Qt::AltModifier);
    QCOMPARE(actionSpy.count(), expectedActionTriggeredCount);
    QCOMPARE(clickSpy.count(), expectedClickedCount);
}

QTEST_QUICKCONTROLS_MAIN(tst_QQuickAbstractButton)

#include "tst_qquickabstractbutton.moc"
