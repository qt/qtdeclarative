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
    QTest::addColumn<bool>("expectEnabledByDefault");
    QTest::addColumn<bool>("expectMnemonicStripped");

    // The data tag is used as the QML type to instantiate; see mnemonics().
    // The "&" should always be stripped from the displayed text for Button and its derived types,
    // regardless of whether the platform supports mnemonics. For example, on Linux, the & is
    // stripped and the next character underlined. On macOS, the & is just stripped.
    QTest::newRow("Button") << platformSupportsMnemonics << true;
    QTest::newRow("CheckBox") << platformSupportsMnemonics << true;
    QTest::newRow("MenuItem") << platformSupportsMnemonics << true;
    QTest::newRow("MenuBarItem") << platformSupportsMnemonics << true;
    QTest::newRow("RadioButton") << platformSupportsMnemonics << true;
    QTest::newRow("Switch") << platformSupportsMnemonics << true;
    QTest::newRow("ToolButton") << platformSupportsMnemonics << true;
    // These types explicitly disable mnemonics regardless of platform.
    QTest::newRow("ItemDelegate") << false << false;
    QTest::newRow("CheckDelegate") << false << false;
    QTest::newRow("RadioDelegate") << false << false;
    QTest::newRow("SwipeDelegate") << false << false;
    QTest::newRow("SwitchDelegate") << false << false;
    QTest::newRow("DelayButton") << false << false;
}

void tst_QQuickAbstractButton::mnemonics()
{
    QFETCH(bool, expectEnabledByDefault);
    QFETCH(bool, expectMnemonicStripped);
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

    // "&" is stripped for non-delegate types on every platform; whether the following
    // character is underlined depends on QPlatformTheme::UnderlineShortcut.
    const bool expectMnemonicUnderlined = expectMnemonicStripped && platformUnderlinesShortcuts;

    QCOMPARE(textItemPrivate->text, expectMnemonicStripped ? "Mnemonic" : "M&nemonic");
    // There should only be one QTextLayout::FormatRange applied to the text: the one for the
    // underline.
    QCOMPARE(textItemPrivate->layout.formats().size(), expectMnemonicUnderlined ? 1 : 0);
    if (expectMnemonicUnderlined) {
        const QTextLayout::FormatRange underlineFormatRange
            = textItemPrivate->layout.formats().constFirst();
        QCOMPARE(underlineFormatRange.start, 1);
        QCOMPARE(underlineFormatRange.length, 1);
        QVERIFY(underlineFormatRange.format.fontUnderline());
    }

    // Set the text to something else to ensure that it picks up changes.
    control->setText(QStringLiteral("&Hello"));
    QCOMPARE(control->text(), QStringLiteral("&Hello"));
    QCOMPARE(textItemPrivate->text, expectMnemonicStripped ? "Hello" : "&Hello");

    const QSignalSpy clickSpy(control, SIGNAL(clicked()));
    QVERIFY(clickSpy.isValid());
    int expectedClickedCount = 0;

    // Pressing the shortcut while visible should click the button.
    if (expectEnabledByDefault)
        ++expectedClickedCount;
    QTest::keyClick(view.get(), Qt::Key_H, Qt::AltModifier);
    QCOMPARE(clickSpy.count(), expectedClickedCount);

    // Pressing the shortcut while hidden shouldn't click the button.
    control->setVisible(false);
    QTest::keyClick(view.get(), Qt::Key_H, Qt::AltModifier);
    QCOMPARE(clickSpy.count(), expectedClickedCount);

    // Pressing the shortcut after showing should click the button.
    if (expectEnabledByDefault)
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
    if (expectEnabledByDefault)
        ++expectedClickedCount;
    QTest::keyClick(view.get(), Qt::Key_S, Qt::AltModifier);
    QCOMPARE(clickSpy.count(), expectedClickedCount);

    // Hide the button and change the shortcut back to the original one; shouldn't click.
    control->setVisible(false);
    control->setText(QStringLiteral("&Hidden"));
    QTest::keyClick(view.get(), Qt::Key_H, Qt::AltModifier);
    QCOMPARE(clickSpy.count(), expectedClickedCount);

    // Restore visibility; should click.
    if (expectEnabledByDefault)
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
    if (expectEnabledByDefault) {
        ++expectedClickedCount;
        ++expectedActionTriggeredCount;
    }
    QTest::keyClick(view.get(), Qt::Key_A, Qt::AltModifier);
    QCOMPARE(actionSpy.count(), expectedActionTriggeredCount);
    QCOMPARE(clickSpy.count(), expectedClickedCount);
}

QTEST_QUICKCONTROLS_MAIN(tst_QQuickAbstractButton)

#include "tst_qquickabstractbutton.moc"
