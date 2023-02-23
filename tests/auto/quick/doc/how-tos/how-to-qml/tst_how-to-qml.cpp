// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/qregularexpression.h>
#include <QtTest/QtTest>
#include <QtQml/qqmlapplicationengine.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuickTemplates2/private/qquicktextfield_p.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>

QT_BEGIN_NAMESPACE

using namespace QQuickControlsTestUtils;

class tst_HowToQml : public QObject
{
    Q_OBJECT

public:
    tst_HowToQml();

private slots:
    void init();
    void activeFocusDebugging();
};

tst_HowToQml::tst_HowToQml()
{
}

void tst_HowToQml::init()
{
    QTest::failOnWarning(QRegularExpression(QStringLiteral(".?")));
}

void tst_HowToQml::activeFocusDebugging()
{
    QQmlApplicationEngine engine;
    engine.loadFromModule("HowToQml", "ActiveFocusDebuggingMain");
    QCOMPARE(engine.rootObjects().size(), 1);

    auto *window = qobject_cast<QQuickWindow*>(engine.rootObjects().at(0));
    window->show();
    QTest::ignoreMessage(QtDebugMsg, QRegularExpression("activeFocusItem: .*\"ActiveFocusDebuggingMain\""));
    QVERIFY(QTest::qWaitForWindowActive(window));

    QTest::ignoreMessage(QtDebugMsg, QRegularExpression("activeFocusItem: .*\"textField1\""));
    auto *textField1 = window->findChild<QQuickTextField*>("textField1");
    QVERIFY(textField1);
    textField1->forceActiveFocus();
    QVERIFY(textField1->hasActiveFocus());

    QTest::ignoreMessage(QtDebugMsg, QRegularExpression("activeFocusItem: .*\"textField2\""));
    auto *textField2 = window->findChild<QQuickTextField*>("textField2");
    QVERIFY(textField2);
    QTest::keyClick(window, Qt::Key_Tab);
    QVERIFY(textField2->hasActiveFocus());
}

QT_END_NAMESPACE

QTEST_MAIN(tst_HowToQml)

#include "tst_how-to-qml.moc"
