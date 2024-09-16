// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    id: root
    width: 640
    height: 480

    property bool finished

    Rectangle {
        id: rect
        color: "red"
        width: 300
        height: 300
        Text {
            text: "Hello world"
            anchors.centerIn: parent
            color: "#00FF00"
        }
        layer.enabled: true
        layer.effect: ShaderEffect {
            objectName: "shaderEffect"
            fragmentShader: "qrc:/data/testprop.frag"

            // This is intended to exercise the uniform data copy logic in
            // QSGRhiShaderEffectMaterialShader::updateUniformData() to see if
            // the undocumented combinations for passing in a vector with more
            // components than the shader variable works, and no assertions or
            // crashes occur.

            property variant aVec4: Qt.point(0.1, 0.0) // third and fourth are 0
            property variant aVec3: Qt.point(0.1, 0.5) // third will be 0
            property variant aVec2: Qt.vector4d(0.1, 0.5, 1.0, 1.0) // only first two are used
            property real f: 0.0
            property variant aFloat: Qt.size(0.1 + f, 0.1) // only first is used

            NumberAnimation on f {
                from: 0.0
                to: 1.0
                duration: 1000
                onFinished: root.finished = true
            }
        }
    }
}
