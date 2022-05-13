// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

Rectangle {
    property string sectionProperty: "section"
    property int sectionPositioning: ViewSection.InlineLabels

    width: 640
    height: 480
    color: "#FFFFFF"

    resources: [
        Component {
            id: myDelegate
            Text {
                objectName: model.title
                width: parent.width
                height: 40
                text: "NormalDelegate: " + model.title
                visible: model.isVisible
                verticalAlignment: Text.AlignVCenter
            }
        }
    ]
    ListView {
        id: list
        objectName: "list"
        anchors.fill: parent
        clip: true

        model: ListModel {
            ListElement {
                title: "element1"
                isVisible: true
                section: "section1"
            }
            ListElement {
                title: "element2"
                isVisible: true
                section: "section1"
            }
            ListElement {
                title: "element3"
                isVisible: true
                section: "section2"
            }
            ListElement {
                title: "element4"
                isVisible: true
                section: "section2"
            }
        }

        delegate: myDelegate

        section.property: "section"
        section.criteria: ViewSection.FullString
        section.delegate: Component {
            Text {
                id: sectionDelegate
                objectName: section
                visible: false
                width: parent.width
                height: visible ? 48 : 0
                text: "Section delegate: " + section
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideMiddle
                Component.onCompleted: function(){
                     Qt.callLater(function(){sectionDelegate.visible = true})
                }
            }
        }
    }
}
