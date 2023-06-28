// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

// This example shows how a ListView can be separated into sections using
// the ListView.section attached property.

import QtQuick
import QtQuick.Controls

Rectangle {
    id: container
    width: 300
    height: 360

    ListModel {
        id: animalsModel

        ListElement {
            name: "Ant"
            size: "Tiny"
        }
        ListElement {
            name: "Flea"
            size: "Tiny"
        }
        ListElement {
            name: "Parrot"
            size: "Small"
        }
        ListElement {
            name: "Guinea pig"
            size: "Small"
        }
        ListElement {
            name: "Rat"
            size: "Small"
        }
        ListElement {
            name: "Butterfly"
            size: "Small"
        }
        ListElement {
            name: "Dog"
            size: "Medium"
        }
        ListElement {
            name: "Cat"
            size: "Medium"
        }
        ListElement {
            name: "Pony"
            size: "Medium"
        }
        ListElement {
            name: "Koala"
            size: "Medium"
        }
        ListElement {
            name: "Horse"
            size: "Large"
        }
        ListElement {
            name: "Tiger"
            size: "Large"
        }
        ListElement {
            name: "Giraffe"
            size: "Large"
        }
        ListElement {
            name: "Elephant"
            size: "Huge"
        }
        ListElement {
            name: "Whale"
            size: "Huge"
        }
    }

//! [0]
    // The delegate for each section header
    Component {
        id: sectionHeading
        Rectangle {
            width: ListView.view.width
            height: childrenRect.height
            color: "lightsteelblue"

            required property string section

            Text {
                text: parent.section
                font.bold: true
                font.pixelSize: 20
            }
        }
    }

    ListView {
        id: view
        anchors.top: parent.top
        anchors.bottom: buttonBar.top
        width: parent.width
        model: animalsModel
        delegate: Text {
            required property string name

            text: name
            font.pixelSize: 18
        }

        section.property: "size"
        section.criteria: ViewSection.FullString
        section.delegate: sectionHeading
    }
//! [0]

    Row {
        id: buttonBar
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 1
        spacing: 1

        CheckBox {
            id: labelAtStartCheckBox
            text: qsTr("CurrentLabelAtStart")
            onClicked: {
                if (checked)
                    view.section.labelPositioning |= ViewSection.CurrentLabelAtStart
                else
                    view.section.labelPositioning &= ~ViewSection.CurrentLabelAtStart
            }
        }

        CheckBox {
            id: labelAtEndCheckBox
            text: qsTr("NextLabelAtEnd")
            onClicked: {
                if (checked)
                    view.section.labelPositioning |= ViewSection.NextLabelAtEnd
                else
                    view.section.labelPositioning &= ~ViewSection.NextLabelAtEnd
            }
        }
    }
}

