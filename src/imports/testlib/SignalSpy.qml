/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtTest 1.0

Item {
    id: spy
    visible: false

    TestUtil {
        id: util
    }

    // Public API.

    property variant target: null
    property string signalName: ""
    property int count: 0

    function clear() {
        count = 0
        qtest_expectedCount = 0
    }

    function wait(timeout) {
        if (timeout === undefined)
            timeout = 5000
        var expected = ++qtest_expectedCount
        var i = 0
        while (i < timeout && count < expected) {
            qtest_results.wait(50)
            i += 50
        }
        var success = (count >= expected)
        if (!qtest_results.verify(success, "wait for signal " + signalName, util.callerFile(), util.callerLine()))
            throw new Error("QtQuickTest::fail")
    }

    // Internal implementation detail follows.

    TestResult { id: qtest_results }

    onTargetChanged: {
        qtest_update()
    }
    onSignalNameChanged: {
        qtest_update()
    }

    property variant qtest_prevTarget: null
    property string qtest_prevSignalName: ""
    property int qtest_expectedCount: 0

    function qtest_update() {
        if (qtest_prevTarget != null) {
            qtest_prevTarget[qtest_prevSignalName].disconnect(spy, "qtest_activated")
            qtest_prevTarget = null
            qtest_prevSignalName = ""
        }
        if (target != null && signalName != "") {
            var func = target[signalName]
            if (func === undefined) {
                console.log("Signal '" + signalName + "' not found")
            } else {
                qtest_prevTarget = target
                qtest_prevSignalName = signalName
                func.connect(spy.qtest_activated)
            }
        }
    }

    function qtest_activated() {
        ++count
    }
}
