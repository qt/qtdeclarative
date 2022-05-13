// Copyright (C) 2016 Gunnar Sletta <gunnar@sletta.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick 2.0
import QtTest 1.1

Item {
    id: root

    width: 100
    height: 62

    Rectangle {
        id: rect
        anchors.fill: parent
        color: "red"
        visible: false
    }

    Component {
        id: component;
        ShaderEffectSource { anchors.fill: parent }
    }

    property var source: undefined;

    Timer {
        id: timer
        interval: 100
        running: true
        onTriggered: {
            var source = component.createObject();
            source.sourceItem = rect;
            source.parent = root;
            root.source = source;
        }
    }

    TestCase {
        id: testcase
        name: "shadersource-dynamic-shadersource"
        when: root.source != undefined

        function test_endresult() {
            if ((Qt.platform.pluginName === "offscreen")
                || (Qt.platform.pluginName === "minimal"))
                skip("grabImage does not work on offscreen/minimal platforms");

            if ((Qt.platform.pluginName === "xcb"))
                skip("grabImage crashes on the xcb platform");

            var image = grabImage(root);
            compare(image.red(0,0), 255);
            compare(image.green(0,0), 0);
            compare(image.blue(0,0), 0);
        }

    }
}
