// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
import QtQuick
import Shapes

Item {
    width: 640; height: 480
    visible: true
    Item {
        anchors.fill: parent

        EllipseItemCpp {
            anchors.fill: parent
        }
        FunkyItemCpp {
            anchors.fill: parent
        }
        FunkyItemQml {
            anchors.fill: parent
        }
    }
}
