// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma ComponentBehavior: Bound

import QtQuick
import WearableStyle
import QtQuick.Effects

import "navigation.js" as NavigationData

Item {
    id: navigationPage

    Image {
        id: view
        anchors.fill: parent
        source: "qrc:/qt/qml/Wearable/images/fallbackmap.png"

        Item {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 10
            anchors.topMargin: 60

            height: dummyWarning.implicitHeight + 20

            opacity: 0.8

            Rectangle {
                id: dummyWarningBg
                anchors.fill: parent
                radius: 20
                color: UIStyle.colorRed
            }

            MultiEffect {
                source: dummyWarningBg
                anchors.fill: parent
                shadowEnabled: true
                shadowBlur: 0.3
                shadowHorizontalOffset: 2
                shadowVerticalOffset: 2
                opacity: 0.5
            }

            Text {
                id: dummyWarning
                text: qsTr("Fallback mode. Install QtLocation for the real mapping experience.")
                font: UIStyle.h3
                color: UIStyle.textColor
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 10
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }
        }

    }

    ListModel {
        id: routeInfoModel
    }

    ListView {
        id: routeView

        height: 90
        anchors.bottom: navigationPage.bottom
        anchors.left: navigationPage.left
        anchors.right: navigationPage.right
        anchors.rightMargin: 10
        anchors.leftMargin: 15
        anchors.bottomMargin: 20

        spacing: 12

        clip: true
        focus: true

        boundsBehavior: Flickable.StopAtBounds
        snapMode: ListView.SnapToItem

        property int centeredIndex: 0

        model: routeInfoModel

        delegate: RouteElement {
            width: routeView.width - 10
            height: routeView.height - 10
        }
    }

    Component.onCompleted: {
        NavigationData.requestNavigationRoute(routeInfoModel)
    }
}
