/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.2
import QtTest 1.0
import QtQuick.Controls 2.0

TestCase {
    id: testCase
    width: 400
    height: 400
    visible: true
    when: windowShown
    name: "ProgressBar"

    Component {
        id: progressBar
        ProgressBar { }
    }

    function test_defaults() {
        var control = progressBar.createObject(testCase)
        compare(control.value, 0)
        compare(control.visualPosition, 0)
        compare(control.indeterminate, false)
        compare(control.layoutDirection, Qt.LeftToRight)
        compare(control.effectiveLayoutDirection, Qt.LeftToRight)
        control.destroy()
    }

    function test_value() {
        var control = progressBar.createObject(testCase, {value: 0.5})
        compare(control.value, 0.5)
        control.value = 1.0
        compare(control.value, 1.0)
        control.value = -1.0
        compare(control.value, 0.0)
        control.value = 2.0
        compare(control.value, 1.0)
        control.destroy()
    }

    function test_layoutDirection() {
        var control = progressBar.createObject(testCase)

        verify(!control.LayoutMirroring.enabled)
        compare(control.layoutDirection, Qt.LeftToRight)
        compare(control.effectiveLayoutDirection, Qt.LeftToRight)

        control.layoutDirection = Qt.RightToLeft
        compare(control.layoutDirection, Qt.RightToLeft)
        compare(control.effectiveLayoutDirection, Qt.RightToLeft)

        control.LayoutMirroring.enabled = true
        compare(control.layoutDirection, Qt.RightToLeft)
        compare(control.effectiveLayoutDirection, Qt.LeftToRight)

        control.layoutDirection = Qt.LeftToRight
        compare(control.layoutDirection, Qt.LeftToRight)
        compare(control.effectiveLayoutDirection, Qt.RightToLeft)

        control.LayoutMirroring.enabled = false
        compare(control.layoutDirection, Qt.LeftToRight)
        compare(control.effectiveLayoutDirection, Qt.LeftToRight)

        control.destroy()
    }

    function test_visualPosition() {
        var control = progressBar.createObject(testCase, {value: 0.25})
        compare(control.value, 0.25)
        compare(control.visualPosition, 0.25)

        control.layoutDirection = Qt.RightToLeft
        compare(control.visualPosition, 0.75)

        control.LayoutMirroring.enabled = true
        compare(control.visualPosition, 0.25)

        control.layoutDirection = Qt.LeftToRight
        compare(control.visualPosition, 0.75)

        control.LayoutMirroring.enabled = false
        compare(control.visualPosition, 0.25)

        control.destroy()
    }
}
