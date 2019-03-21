/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/qtest.h>
#include <QtQml/qqmlengine.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/qquickview.h>
#include <QtQuickTest/quicktest.h>

#include "../../shared/util.h"

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
