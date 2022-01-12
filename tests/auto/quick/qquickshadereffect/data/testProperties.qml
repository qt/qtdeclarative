/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
