// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtCore/qregularexpression.h>
#include <QtTest/QtTest>
#include <QtQml/qqmlapplicationengine.h>
#include <QtQuick/qquickwindow.h>
#include <QtQuickTemplates2/private/qquickbutton_p.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>

QT_BEGIN_NAMESPACE

using namespace QQuickControlsTestUtils;

class tst_HowToCppButton : public QObject
{
    Q_OBJECT

public:
    tst_HowToCppButton();

private slots:
    void example();
};

tst_HowToCppButton::tst_HowToCppButton()
{
}

void tst_HowToCppButton::example()
{
    QTest::failOnWarning(QRegularExpression(QStringLiteral(".?")));

    QQmlApplicationEngine engine;
    engine.loadFromModule("MyModule", "Main");
    QCOMPARE(engine.rootObjects().size(), 1);

    auto *window = qobject_cast<QQuickWindow*>(engine.rootObjects().at(0));
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto *button = window->findChild<QQuickButton*>();
    QVERIFY(button);
    QCOMPARE(button->text(), "Click me");
    QTest::ignoreMessage(QtDebugMsg, "Did stuff!");
    QVERIFY(clickButton(button));
}

QT_END_NAMESPACE

QTEST_MAIN(tst_HowToCppButton)

#include "tst_how-to-cpp-button.moc"
