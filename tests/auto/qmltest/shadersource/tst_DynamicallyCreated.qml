// Copyright (C) 2016 Jolla Ltd, author: <gunnar.sletta@jollamobile.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.2
import QtQuick.Window 2.0
import QtTest 1.1

Item {
    width: 100
    height: 100

    Window {
        id: win

        width: 100
        height: 100

        property bool rendered: false;
        visible: true

        title: "QML window"

        onFrameSwapped: {
            if (shaderSource.sourceItem) {
                rendered = true;
            } else {
                var com = Qt.createQmlObject('import QtQuick 2.2; Rectangle { color: "red"; width: 100; height: 100 }', win);
                shaderSource.sourceItem = com;
            }
        }

        ShaderEffectSource {
            id: shaderSource
        }

    }

    TestCase {
        when: win.rendered;
        name: "shadersource-dynamic-sourceobject"
        function test_endresult() {
            var image = grabImage(shaderSource);
            compare(image.red(0,0), 255);
            compare(image.green(0,0), 0);
            compare(image.blue(0,0), 0);
        }
    }
}
