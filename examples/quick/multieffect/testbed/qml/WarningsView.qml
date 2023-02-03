// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick

Item {
    id: rootItem

    function showShaderWarning() {
        shaderWarning.show();
    }
    function showSizeWarning() {
        sizeWarning.show();
    }

    height: 60

    WarningsItem {
        id: shaderWarning
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        text: qsTr("Shader changed!")
    }
    WarningsItem {
        id: sizeWarning
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: shaderWarning.right
        anchors.leftMargin: 16
        text: qsTr("Item resized!")
    }
    WarningsItem {
        id: customEffectWarning
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        text: qsTr("<b>Note:</b> Custom MultiEffect doesn't support all the MultiEffect features, like paddings and disabling effects.")
        opacity: settings.showCustomMultiEffect
    }
}
