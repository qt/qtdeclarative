/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
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
    name: "Control"

    Component {
        id: component
        Control { }
    }

    SignalSpy {
        id: layoutDirectionSpy
        signalName: "layoutDirectionChanged"
    }

    SignalSpy {
        id: effectiveLayoutDirectionSpy
        signalName: "effectiveLayoutDirectionChanged"
    }

    SignalSpy {
        id: mirroredSpy
        signalName: "mirroredChanged"
    }

    function test_defaults() {
        var control = component.createObject(testCase)
        verify(control)
        compare(control.layoutDirection, Qt.LeftToRight)
        compare(control.effectiveLayoutDirection, Qt.LeftToRight)
        compare(control.mirrored, false)
        compare(control.padding, 0)
        compare(control.topPadding, 0)
        compare(control.leftPadding, 0)
        compare(control.rightPadding, 0)
        compare(control.bottomPadding, 0)
        compare(control.availableWidth, 0)
        compare(control.availableHeight, 0)
        control.destroy()
    }

    function test_padding() {
        var control = component.createObject(testCase)

        control.width = 100
        control.height = 100

        control.padding = 10
        compare(control.padding, 10)
        compare(control.topPadding, 10)
        compare(control.leftPadding, 10)
        compare(control.rightPadding, 10)
        compare(control.bottomPadding, 10)

        control.topPadding = 20
        compare(control.padding, 10)
        compare(control.topPadding, 20)
        compare(control.leftPadding, 10)
        compare(control.rightPadding, 10)
        compare(control.bottomPadding, 10)

        control.leftPadding = 30
        compare(control.padding, 10)
        compare(control.topPadding, 20)
        compare(control.leftPadding, 30)
        compare(control.rightPadding, 10)
        compare(control.bottomPadding, 10)

        control.rightPadding = 40
        compare(control.padding, 10)
        compare(control.topPadding, 20)
        compare(control.leftPadding, 30)
        compare(control.rightPadding, 40)
        compare(control.bottomPadding, 10)

        control.bottomPadding = 50
        compare(control.padding, 10)
        compare(control.topPadding, 20)
        compare(control.leftPadding, 30)
        compare(control.rightPadding, 40)
        compare(control.bottomPadding, 50)

        control.padding = 60
        compare(control.padding, 60)
        compare(control.topPadding, 20)
        compare(control.leftPadding, 30)
        compare(control.rightPadding, 40)
        compare(control.bottomPadding, 50)

        control.destroy()
    }

    function test_availableSize() {
        var control = component.createObject(testCase)

        control.width = 100
        control.height = 100
        compare(control.availableWidth, 100)
        compare(control.availableHeight, 100)

        control.padding = 10
        compare(control.availableWidth, 80)
        compare(control.availableHeight, 80)

        control.topPadding = 20
        compare(control.availableWidth, 80)
        compare(control.availableHeight, 70)

        control.leftPadding = 30
        compare(control.availableWidth, 60)
        compare(control.availableHeight, 70)

        control.rightPadding = 40
        compare(control.availableWidth, 30)
        compare(control.availableHeight, 70)

        control.bottomPadding = 50
        compare(control.availableWidth, 30)
        compare(control.availableHeight, 30)

        control.padding = 60
        compare(control.availableWidth, 30)
        compare(control.availableHeight, 30)

        control.destroy()
    }

    function test_layoutDirection() {
        var control = component.createObject(testCase)

        layoutDirectionSpy.target = control
        effectiveLayoutDirectionSpy.target = control
        mirroredSpy.target = control

        verify(layoutDirectionSpy.valid)
        verify(effectiveLayoutDirectionSpy.valid)
        verify(mirroredSpy.valid)

        verify(!control.LayoutMirroring.enabled)
        compare(control.layoutDirection, Qt.LeftToRight)
        compare(control.effectiveLayoutDirection, Qt.LeftToRight)
        compare(control.mirrored, false)

        control.layoutDirection = Qt.RightToLeft
        compare(control.layoutDirection, Qt.RightToLeft)
        compare(control.effectiveLayoutDirection, Qt.RightToLeft)
        compare(control.mirrored, true)
        compare(layoutDirectionSpy.count, 1)
        compare(effectiveLayoutDirectionSpy.count, 1)
        compare(mirroredSpy.count, 1)

        control.LayoutMirroring.enabled = true
        compare(control.layoutDirection, Qt.RightToLeft)
        compare(control.effectiveLayoutDirection, Qt.LeftToRight)
        compare(control.mirrored, false)
        compare(layoutDirectionSpy.count, 1)
        compare(effectiveLayoutDirectionSpy.count, 2)
        compare(mirroredSpy.count, 2)

        control.layoutDirection = Qt.LeftToRight
        compare(control.layoutDirection, Qt.LeftToRight)
        compare(control.effectiveLayoutDirection, Qt.RightToLeft)
        compare(control.mirrored, true)
        compare(layoutDirectionSpy.count, 2)
        compare(effectiveLayoutDirectionSpy.count, 3)
        compare(mirroredSpy.count, 3)

        control.LayoutMirroring.enabled = false
        compare(control.layoutDirection, Qt.LeftToRight)
        compare(control.effectiveLayoutDirection, Qt.LeftToRight)
        compare(control.mirrored, false)
        compare(layoutDirectionSpy.count, 2)
        compare(effectiveLayoutDirectionSpy.count, 4)
        compare(mirroredSpy.count, 4)

        control.destroy()
    }

    function test_background() {
        var control = component.createObject(testCase)

        control.background = component.createObject(control)

        // background has no x or width set, so its width follows control's width
        control.width = 320
        compare(control.background.width, control.width)

        // background has no y or height set, so its height follows control's height
        compare(control.background.height, control.height)
        control.height = 240

        // has width => width does not follow
        control.background.width /= 2
        control.width += 20
        verify(control.background.width !== control.width)

        // reset width => width follows again
        control.background.width = undefined
        control.width += 20
        compare(control.background.width, control.width)

        // has x => width does not follow
        control.background.x = 10
        control.width += 20
        verify(control.background.width !== control.width)

        // has height => height does not follow
        control.background.height /= 2
        control.height -= 20
        verify(control.background.height !== control.height)

        // reset height => height follows again
        control.background.height = undefined
        control.height -= 20
        compare(control.background.height, control.height)

        // has y => height does not follow
        control.background.y = 10
        control.height -= 20
        verify(control.background.height !== control.height)

        control.destroy()
    }
}
