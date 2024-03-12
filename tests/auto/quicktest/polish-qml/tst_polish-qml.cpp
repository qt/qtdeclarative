// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtQml/qqmlengine.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/qquickview.h>
#include <QtQuickTest/quicktest.h>

class CustomItem : public QQuickItem
{
    Q_OBJECT

public:
    CustomItem() {}

    Q_INVOKABLE void polishMe() {
        polish();
    }

    Q_INVOKABLE bool isPolishScheduled() const
    {
        return QQuickItemPrivate::get(this)->polishScheduled;
    }

    Q_INVOKABLE bool wasUpdatePolishCalled() const
    {
        return updatePolishCalled;
    }

    Q_INVOKABLE void clearUpdatePolishCalled() {
        updatePolishCalled = false;
    }

    void updatePolish() override
    {
        updatePolishCalled = true;
    }

private:
    bool updatePolishCalled = false;
};

class TestSetup : public QObject
{
    Q_OBJECT

public:
    TestSetup() {}

public slots:
    void applicationAvailable()
    {
        qmlRegisterType<CustomItem>("Test", 1, 0, "CustomItem");
    }
};

QUICK_TEST_MAIN_WITH_SETUP(polish-qml, TestSetup)

#include "tst_polish-qml.moc"
