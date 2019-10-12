/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
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

import QtQuick 2.12
import QtQuick.Window 2.2
import QtTest 1.1
import QtQuick.Templates 2.12 as T
import QtQuick.Controls 2.12
import QtQuick.Controls.Imagine 2.12
import QtQuick.Controls.Imagine.impl 2.12

TestCase {
    id: testCase
    width: 200
    height: 200
    visible: true
    when: windowShown
    name: "Imagine"

    Component {
        id: buttonComponent
        Button {}
    }

    Component {
        id: implicitQrcButtonComponent
        Button {
            Imagine.path: ":/control-assets"
        }
    }

    Component {
        id: explicitQrcButtonComponent
        Button {
            Imagine.path: "qrc:/control-assets"
        }
    }

    function test_qrcPaths_data() {
        return [
            { tag: ":/control-assets", component: implicitQrcButtonComponent },
            { tag: "qrc:/control-assets", component: explicitQrcButtonComponent }
        ]
    }

    function test_qrcPaths(data) {
        if (Qt.platform.pluginName === "offscreen")
            skip("grabImage() is not functional on the offscreen platform (QTBUG-63185)")

        var control = createTemporaryObject(data.component, testCase)
        verify(control)
        compare(control.Imagine.path, data.tag)
        var image = grabImage(control)
        compare(image.pixel(control.width / 2, control.height / 2), "#ff0000")
    }

    function test_fontFromConfigFile() {
        var control = createTemporaryObject(buttonComponent, testCase)
        verify(control)
        compare(control.font.pixelSize, 80)
    }

    Component {
        id: ninePatchImageComponent

        NinePatchImage {
            property alias mouseArea: mouseArea

            MouseArea {
                id: mouseArea
                anchors.fill: parent
                // The name of the images isn't important; we just want to check that
                // going from regular to 9-patch to regular to regular works without crashing.
                onPressed: parent.source = "qrc:/control-assets/button-background.9.png"
                onReleased: parent.source = "qrc:/test-assets/button-background-1.png"
                onClicked: parent.source = "qrc:/test-assets/button-background-2.png"
            }
        }
    }

    Component {
        id: signalSpyComponent

        SignalSpy {}
    }

    // QTBUG-78790
    function test_switchBetween9PatchAndRegular() {
        var ninePatchImage = createTemporaryObject(ninePatchImageComponent, testCase,
            { source: "qrc:/test-assets/button-background-1.png" })
        verify(ninePatchImage)

        var clickSpy = signalSpyComponent.createObject(ninePatchImage,
            { target: ninePatchImage.mouseArea, signalName: "clicked" })
        verify(clickSpy.valid)

        var afterRenderingSpy = signalSpyComponent.createObject(ninePatchImage,
            { target: testCase.Window.window, signalName: "afterRendering" })
        verify(afterRenderingSpy.valid)

        mousePress(ninePatchImage)
        // Wait max 1 second - in reality it should take a handful of milliseconds.
        afterRenderingSpy.wait(1000)
        mouseRelease(ninePatchImage)
        compare(clickSpy.count, 1)
        // Shouldn't result in a crash.
        afterRenderingSpy.wait(1000)
    }
}
