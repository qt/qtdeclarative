// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import WearableStyle
import "navigation.js" as NavigationData

Item {
    property alias routeListView: routeView

    Column {
        anchors.fill: parent
        anchors.margins: 2
        spacing: 2

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: parent.width
            height: titleRow.height

            color: UIStyle.themeColorQtGray9

            Row {
                id: titleRow
                spacing: 10
                anchors.centerIn: parent

                Image {
                    anchors.verticalCenter: parent.verticalCenter
                    source: UIStyle.themeImagePath("navigation")
                    fillMode: Image.PreserveAspectCrop
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Walking")
                    font.pixelSize: UIStyle.fontSizeM
                    font.letterSpacing: 2
                    color: UIStyle.themeColorQtGray2
                }
            }
        }

        ListModel {
            id: routeModel
        }

        ListView {
            id: routeView

            width: parent.width
            height: parent.height - titleRow.height - parent.spacing
            property var imageList: [UIStyle.themeImagePath("navigation-straight"),
                                     UIStyle.themeImagePath("navigation-leftturn"),
                                     UIStyle.themeImagePath("navigation-rightturn"),
                                     UIStyle.imagePath("navigation-uturn"),
                                     UIStyle.imagePath("navigation-start"),
                                     UIStyle.imagePath("navigation-end")]

            clip: true
            focus: true
            boundsBehavior: Flickable.StopAtBounds
            snapMode: ListView.SnapToItem
            model: routeModel
            delegate: RouteElement {
                width: routeView.width
                height: routeView.height
            }
        }
    }
    Component.onCompleted: {
        NavigationData.requestNavigationRoute(routeModel)
    }
}
