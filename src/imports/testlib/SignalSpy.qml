/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
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
    property var target: null
    property string signalName: ""
    readonly property alias count: spy.qtest_count
    readonly property alias valid:spy.qtest_valid
    readonly property alias signalArguments:spy.qtest_signalArguments

    function clear() {
        qtest_count = 0
        qtest_expectedCount = 0
        qtest_signalArguments = []
    }

    function wait(timeout) {
        if (timeout === undefined)
            timeout = 5000
        var expected = ++qtest_expectedCount
        var i = 0
        while (i < timeout && qtest_count < expected) {
            qtest_results.wait(50)
            i += 50
        }
        var success = (qtest_count >= expected)
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

    property var qtest_prevTarget: null
    property string qtest_prevSignalName: ""
    property int qtest_expectedCount: 0
    property var qtest_signalArguments:[]
    property int qtest_count: 0
    property bool qtest_valid:false

    function qtest_update() {
        if (qtest_prevTarget != null) {
            var prevFunc = qtest_prevTarget[qtest_prevSignalName]
            if (prevFunc)
                prevFunc.disconnect(spy.qtest_activated)
            qtest_prevTarget = null
            qtest_prevSignalName = ""
        }
        if (target != null && signalName != "") {
            var func = target[signalName]
            if (func === undefined) {
                spy.qtest_valid = false
                console.log("Signal '" + signalName + "' not found")
            } else {
                qtest_prevTarget = target
                qtest_prevSignalName = signalName
                func.connect(spy.qtest_activated)
                spy.qtest_valid = true
                spy.qtest_signalArguments = []
            }
        } else {
            spy.qtest_valid = false
        }
    }

    function qtest_activated() {
        ++qtest_count
        spy.qtest_signalArguments[spy.qtest_signalArguments.length] = arguments
    }
}
