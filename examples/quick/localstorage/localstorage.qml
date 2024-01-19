// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.LocalStorage
import "Database.js" as JS

pragma ComponentBehavior: Bound

Window {
    id: window

    property bool creatingNewEntry: false
    property bool editingEntry: false

    visible: true
    width: Screen.width / 2
    height: Screen.height / 1.8
    color: "#161616"

    Rectangle {
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 10

            Header {
                id: input
                Layout.fillWidth: true
                listView: listView
                enabled: window.creatingNewEntry || window.editingEntry
            }

            RowLayout {
                Button {
                    text: qsTr("New")
                    Layout.fillWidth: false
                    onClicked: {
                        input.initrec_new()
                        window.creatingNewEntry = true
                        listView.model.setProperty(listView.currentIndex, "id", 0)
                    }
                }
                Button {
                    id: saveButton
                    enabled: (window.creatingNewEntry || window.editingEntry) && listView.currentIndex !== -1
                    text: qsTr("Save")
                    Layout.fillWidth: false
                    onClicked: {
                        let insertedRow = false;
                        if (listView.model.get(listView.currentIndex).id < 1) {
                            //insert mode
                            if (input.insertrec()) {
                                // Successfully inserted a row.
                                input.setlistview()
                                insertedRow = true
                            } else {
                                // Failed to insert a row; display an error message.
                                statustext.displayWarning(qsTr("Failed to insert row"))
                            }
                        } else {
                            // edit mode
                            input.setlistview()
                            JS.dbUpdate(listView.model.get(listView.currentIndex).date,
                                        listView.model.get(listView.currentIndex).trip_desc,
                                        listView.model.get(listView.currentIndex).distance,
                                        listView.model.get(listView.currentIndex).id)
                        }

                        if (insertedRow) {
                            input.initrec()
                            window.creatingNewEntry = false
                            window.editingEntry = false
                            listView.forceLayout()
                        }
                    }
                }
                Button {
                    id: editButton
                    text: qsTr("Edit")
                    enabled: !window.creatingNewEntry && !window.editingEntry && listView.currentIndex !== -1
                    Layout.fillWidth: false
                    onClicked: {
                        input.editrec(listView.model.get(listView.currentIndex).date,
                                      listView.model.get(listView.currentIndex).trip_desc,
                                      listView.model.get(listView.currentIndex).distance,
                                      listView.model.get(listView.currentIndex).id)

                        window.editingEntry = true
                    }
                }
                Button {
                    id: deleteButton
                    text: qsTr("Delete")
                    enabled: !window.creatingNewEntry && listView.currentIndex !== -1
                    Layout.fillWidth: false
                    onClicked: {
                        JS.dbDeleteRow(listView.model.get(listView.currentIndex).id)
                        listView.model.remove(listView.currentIndex, 1)
                        if (listView.count === 0) {
                            // ListView doesn't automatically set its currentIndex to -1
                            // when the count becomes 0.
                            listView.currentIndex = -1
                        }
                    }
                }
                Button {
                    id: cancelButton
                    text: qsTr("Cancel")
                    enabled: (window.creatingNewEntry || window.editingEntry) && listView.currentIndex !== -1
                    Layout.fillWidth: false
                    onClicked: {
                        if (listView.model.get(listView.currentIndex).id === 0) {
                            // This entry had an id of 0, which means it was being created and hadn't
                            // been saved to the database yet, so we can safely remove it from the model.
                            listView.model.remove(listView.currentIndex, 1)
                        }
                        listView.forceLayout()
                        window.creatingNewEntry = false
                        window.editingEntry = false
                        input.initrec()
                    }
                }
                Button {
                    text: qsTr("Exit")
                    Layout.fillWidth: false
                    onClicked: Qt.quit()
                }
            }
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 5
            }
            Label {
                Layout.alignment: Qt.AlignCenter
                text: qsTr("Saved activities")
                font.pointSize: 15
            }
            Component {
                id: highlightBar
                Rectangle {
                    width: listView.currentItem?.width ?? implicitWidth
                    height: listView.currentItem?.height ?? implicitHeight
                    color: "lightgreen"
                }
            }
            ListView {
                id: listView
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: MyModel {}
                delegate: MyDelegate {
                    width: listView.width
                    onClicked: ()=> listView.currentIndex = index
                }
                // Don't allow changing the currentIndex while the user is creating/editing values.
                enabled: !window.creatingNewEntry && !window.editingEntry

                highlight: highlightBar
                highlightFollowsCurrentItem: true
                focus: true
                clip: true

                header: Component {
                    RowLayout {
                        width: ListView.view.width
                        Repeater {
                            model: [qsTr("Date"), qsTr("Description"), qsTr("Distance")]
                            delegate: Label {
                                id: headerTitleDelegate

                                required property string modelData

                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.preferredWidth: 1
                                text: modelData
                                font {
                                    pointSize: 15
                                    bold: true
                                    underline: true
                                }
                                padding: 12
                                horizontalAlignment: Label.AlignHCenter
                            }
                        }
                    }
                }
            }
            Label {
                id: statustext
                color: "red"
                font.bold: true
                font.pointSize: 20
                opacity: 0.0
                visible: opacity > 0 // properly cull item if effectively invisible
                Layout.alignment: Layout.Center

                function displayWarning(text) {
                    statustext.text = text
                    statusAnim.restart()
                }

                Connections {
                    target: input
                    function onStatusMessage(msg) { statustext.displayWarning(msg); }
                }

                SequentialAnimation {
                    id: statusAnim

                    OpacityAnimator {
                        target: statustext
                        from: 0.0
                        to: 1.0
                        duration: 50
                    }

                    PauseAnimation {
                        duration: 2000
                    }

                    OpacityAnimator {
                        target: statustext
                        from: 1.0
                        to: 0.0
                        duration: 50
                    }
                }
            }
        }
    }
    Component.onCompleted: {
        JS.dbInit()
    }
}
