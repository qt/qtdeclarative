// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtCore
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Window {
    id: window
    visible: true
    title: qsTr("Qt QML Permissions")

    readonly property int margin: 11

    width: layout.implicitWidth + 2 * margin
    height: layout.implicitHeight + 2 * margin

    Rectangle {
        anchors.fill: parent
        color: locationPermission.status === Qt.Granted ? "green" :
               locationPermission.status === Qt.Denied ? "red" : "blue"

        LocationPermission {
            id: locationPermission

            // If any of the properties of the permission are changed,
            // checkPermision is used to check, and the status will
            // update accordingly. For example, upgrading from Approximate
            // to Precise would either result in Undetermined, if we
            // can still request Precise, or Denied, if we already did,
            // or it's not possible to upgrade the permission.
            accuracy: accuracyCombo.currentValue === qsTr("Precise") ? LocationPermission.Precise
                                                                     : LocationPermission.Approximate
            availability: availabilityCombo.currentValue === qsTr("Always") ? LocationPermission.Always
                                                                            : LocationPermission.WhenInUse
            // However, the permission will not be affected by other
            // permissions of the same type. In other words, a more
            // extensive permission being granted does not reflect
            // in this permission. Nor does losing an already granted
            // permission, for example due to the user updating their
            // system settings.
            onStatusChanged: console.log("Status: " + status)
            onAccuracyChanged: console.log("Accuracy: " + accuracy)
            onAvailabilityChanged: console.log("Availability: " + availability)
        }

        ColumnLayout {
            id: layout

            anchors.fill: parent
            anchors.margins: window.margin

            Text {
                readonly property string statusAsString: locationPermission.status === Qt.Granted ? qsTr("Granted") :
                                                         locationPermission.status === Qt.Undetermined ? qsTr("Undetermined")
                                                                                                       : qsTr("Denied")
                text: qsTr(`Location services, status: ${statusAsString}`)

                font.bold: true
                font.pointSize: 16
                color: locationPermission.status === Qt.Undetermined ? "white" : "black"

                Layout.alignment: Qt.AlignHCenter
            }

            RowLayout {
                ComboBox {
                    id: accuracyCombo

                    model: ["Precise", "Approximate"]
                    implicitContentWidthPolicy: ComboBox.WidestText
                    Layout.fillWidth: true
                }
                ComboBox {
                    id: availabilityCombo

                    model: ["Always", "When in use"]

                    implicitContentWidthPolicy: ComboBox.WidestText
                    Layout.fillWidth: true
                }
            }

            Button {
                text: qsTr("Request location permissions")
                enabled: locationPermission.status !== Qt.Denied
                Layout.alignment: Qt.AlignHCenter

                onClicked: locationPermission.request()
            }
        }

        Component.onCompleted: {
            console.log(locationPermission.status)
        }
    }
}
