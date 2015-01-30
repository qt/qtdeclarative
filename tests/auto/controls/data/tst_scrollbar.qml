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
    name: "ScrollBar"

    SignalSpy{
        id: pressedSpy
        signalName: "pressedChanged"
    }

    Component {
        id: scrollBar
        ScrollBar { }
    }

    Component {
        id: flickable
        Flickable {
            width: 100
            height: 100
            contentWidth: 200
            contentHeight: 200
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.HorizontalAndVerticalFlick
        }
    }

    function init() {
        verify(!pressedSpy.target)
        compare(pressedSpy.count, 0)
    }

    function cleanup() {
        pressedSpy.target = null
        pressedSpy.clear()
    }

    function test_defaults() {
        var control = scrollBar.createObject(testCase)
        verify(control)
        verify(control.handle)
        compare(control.size, 0.0)
        compare(control.position, 0.0)
        compare(control.active, false)
        compare(control.orientation, Qt.Vertical)
        control.destroy()
    }

    function test_attach() {
        var container = flickable.createObject(testCase)
        waitForRendering(container)

        var vertical = scrollBar.createObject()
        verify(!vertical.parent)
        container.AbstractScrollBar.vertical = vertical
        compare(vertical.parent, container)
        compare(vertical.orientation, Qt.Vertical)
        compare(vertical.size, container.visibleArea.heightRatio)
        compare(vertical.position, container.visibleArea.yPosition)

        var horizontal = scrollBar.createObject()
        verify(!horizontal.parent)
        container.AbstractScrollBar.horizontal = horizontal
        compare(horizontal.parent, container)
        compare(horizontal.orientation, Qt.Horizontal)
        compare(horizontal.size, container.visibleArea.widthRatio)
        compare(horizontal.position, container.visibleArea.xPosition)

        var velocity = container.maximumFlickVelocity

        compare(container.flicking, false)
        container.flick(-velocity, -velocity)
        compare(container.flicking, true)
        tryCompare(container, "flicking", false)

        compare(vertical.size, container.visibleArea.heightRatio)
        compare(vertical.position, container.visibleArea.yPosition)
        compare(horizontal.size, container.visibleArea.widthRatio)
        compare(horizontal.position, container.visibleArea.xPosition)

        compare(container.flicking, false)
        container.flick(velocity, velocity)
        compare(container.flicking, true)
        tryCompare(container, "flicking", false)

        compare(vertical.size, container.visibleArea.heightRatio)
        compare(vertical.position, container.visibleArea.yPosition)
        compare(horizontal.size, container.visibleArea.widthRatio)
        compare(horizontal.position, container.visibleArea.xPosition)

        container.destroy()
    }

    function test_mouse_data() {
        return [
            { tag: "horizontal", properties: { visible: true, orientation: Qt.Horizontal, width: testCase.width } },
            { tag: "vertical", properties: { visible: true, orientation: Qt.Vertical, height: testCase.height } }
        ]
    }

    function test_mouse(data) {
        var control = scrollBar.createObject(testCase, data.properties)

        pressedSpy.target = control
        verify(pressedSpy.valid)

        mousePress(control, 0, 0, Qt.LeftButton)
        compare(pressedSpy.count, 1)
        compare(control.pressed, true)
        compare(control.position, 0.0)

        mouseMove(control, -control.width, -control.height, 0, Qt.LeftButton)
        compare(pressedSpy.count, 1)
        compare(control.pressed, true)
        compare(control.position, 0.0)

        mouseMove(control, control.width * 0.5, control.height * 0.5, 0, Qt.LeftButton)
        compare(pressedSpy.count, 1)
        compare(control.pressed, true)
        verify(control.position, 0.5)

        mouseRelease(control, control.width * 0.5, control.height * 0.5, Qt.LeftButton)
        compare(pressedSpy.count, 2)
        compare(control.pressed, false)
        compare(control.position, 0.5)

        mousePress(control, control.width, control.height, Qt.LeftButton)
        compare(pressedSpy.count, 3)
        compare(control.pressed, true)
        compare(control.position, 0.5)

        mouseMove(control, control.width * 2, control.height * 2, 0, Qt.LeftButton)
        compare(pressedSpy.count, 3)
        compare(control.pressed, true)
        compare(control.position, 1.0)

        mouseMove(control, control.width * 0.75, control.height * 0.75, 0, Qt.LeftButton)
        compare(pressedSpy.count, 3)
        compare(control.pressed, true)
        compare(control.position, 0.75)

        mouseRelease(control, control.width * 0.25, control.height * 0.25, Qt.LeftButton)
        compare(pressedSpy.count, 4)
        compare(control.pressed, false)
        compare(control.position, 0.25)

        control.destroy()
    }
}
