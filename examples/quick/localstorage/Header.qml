// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.LocalStorage
import "Database.js" as JS

Item {
    id: root
    required property ListView listView
    signal statusMessage(string msg)

    width: Screen.width / 2
    height: Screen.height / 7
    enabled: false

    function insertrec() {
        const rowid = parseInt(JS.dbInsert(dateInput.text, descInput.text, distInput.text), 10)
        if (rowid) {
            listView.model.setProperty(listView.currentIndex, "id", rowid)
            listView.forceLayout()
        }
        return rowid;
    }

    function editrec(Pdate, Pdesc, Pdistance, Prowid) {
        dateInput.text = Pdate
        descInput.text = Pdesc
        distInput.text = Pdistance
    }

    function initrec_new() {
        dateInput.clear()
        descInput.clear()
        distInput.clear()
        listView.model.insert(0, {
                                  date: "",
                                  trip_desc: "",
                                  distance: 0
                              })
        listView.currentIndex = 0
        dateInput.forceActiveFocus()
    }

    function initrec() {
        dateInput.clear()
        descInput.clear()
        distInput.clear()
    }

    function setlistview() {
        listView.model.setProperty(listView.currentIndex, "date",
                                   dateInput.text)
        listView.model.setProperty(listView.currentIndex, "trip_desc",
                                   descInput.text)
        listView.model.setProperty(listView.currentIndex, "distance",
                                   parseInt(distInput.text,10))
    }

    Rectangle {
        id: rootrect
        border.width: 10
        color: "#161616"

        ColumnLayout {
            id: mainLayout
            anchors.fill: parent

            Rectangle {
                id: gridBox
                Layout.fillWidth: true

                GridLayout {
                    id: gridLayout
                    rows: 3
                    flow: GridLayout.TopToBottom
                    anchors.fill: parent

                    Label {
                        text: qsTr("Date")
                        font.pixelSize: 22
                        rightPadding: 10
                    }

                    Label {
                        text: qsTr("Description")
                        font.pixelSize: 22
                        rightPadding: 10
                    }

                    Label {
                        text: qsTr("Distance")
                        font.pixelSize: 22
                    }

                    TextField {
                        id: dateInput
                        font.pixelSize: 22
                        activeFocusOnPress: true
                        activeFocusOnTab: true

                        ToolTip {
                            x: parent.width + 3
                            y: (parent.height - height) / 2
                            text: qsTr("Date format = 'YYYY-MM-DD'")
                            visible: dateInput.enabled && dateInput.hovered
                            delay: 1000
                        }

                        validator: RegularExpressionValidator {
                            regularExpression: /\d{4}[,.:/-]\d\d?[,.:/-]\d\d?/
                        }

                        onFocusChanged: function() {
                            if (!dateInput.focus && !acceptableInput && root.enabled)
                                root.statusMessage(qsTr("Please fill in the date"));
                        }

                        onEditingFinished: function() {
                            const regex = /(\d+)[,.:/-](\d+)[,.:/-](\d+)/
                            if (dateInput.text.match(regex))
                                dateInput.text = dateInput.text.replace(regex, '$1-$2-$3')
                        }
                    }

                    TextField {
                        id: descInput
                        property string oldString
                        font.pixelSize: 22
                        activeFocusOnPress: true
                        activeFocusOnTab: true
                        onFocusChanged: if (focus) oldString = descInput.text
                        onEditingFinished: function() {
                            if (descInput.text.length < 8 && descInput.text !== descInput.oldString && root.enabled)
                                root.statusMessage(qsTr("Enter a description of minimum 8 characters"))
                        }
                    }

                    TextField {
                        id: distInput
                        property string oldString
                        font.pixelSize: 22
                        activeFocusOnPress: true
                        activeFocusOnTab: true
                        validator: RegularExpressionValidator {
                            regularExpression: /\d{1,3}/
                        }
                        onFocusChanged: if (focus) oldString = distInput.text
                        onEditingFinished: function() {
                            if (distInput.text === "" && distInput.text !== distInput.oldString && root.enabled)
                                root.statusMessage(qsTr("Please fill in the distance"))
                        }
                    }
                }
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: dateInput.forceActiveFocus()
    }
}
