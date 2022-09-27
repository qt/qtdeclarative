// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Window
import QtTest
import QtQuick.Templates as T
import QtQuick.Controls
import QtQuick.Controls.Imagine
import QtQuick.Controls.Imagine.impl

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

    Component {
        id: invalidNinePatchImageProvider
        Item {
            width: 200
            height: 200
            property alias ninePatchImage: np

            NinePatchImage {
                id: np
                source : "qrc:/test-assets/button-background-1.png"
                cache: false
                visible: false
            }

            ShaderEffect {
                width: 200
                height: 200
                property variant source: np
                property real amplitude: 0.04
                property real frequency: 20
                property real time: 0
                fragmentShader: "qrc:/test-assets/wobble.frag.qsb"
            }
        }
    }

    // QTBUG-100508
    function test_invalidNinePatchImageProvider() {
        var container = createTemporaryObject(invalidNinePatchImageProvider, testCase)
        verify(container);
        var afterRenderingSpy = signalSpyComponent.createObject(null,
            { target: testCase.Window.window, signalName: "afterRendering" })
        verify(afterRenderingSpy.valid)

        afterRenderingSpy.wait(1000)
        container.ninePatchImage.source = ""
        // Shouldn't result in a crash.
        afterRenderingSpy.wait(1000)
    }
}
