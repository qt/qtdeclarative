// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.LocalStorage

Page {
    id: root

    header: Label {
        text: qsTr("Projects")
        font.pointSize: AppSettings.fontSize + 20
        font.styleName: "Semibold"
        leftPadding: 30
        topPadding: 20
        bottomPadding: 20
    }

    ListView {
        id: projectListView
        anchors.fill: parent
        clip: true

        model: ListModel {
            id: projectsModel

            Component.onCompleted: {
                let projects = Database.getProjects()
                for (let project of projects)
                    append(project)
            }
        }

        delegate: SwipeDelegate {
            id: projectDelegate
            width: ListView.view.width
            height: projectContent.implicitHeight

            required property int index
            required property int projectId
            required property string projectName
            required property int completedTasks
            required property int totalTasks

            swipe.right: Rectangle {
                width: 50
                height: parent.height
                color: "red"
                anchors.right: parent.right

                SwipeDelegate.onClicked: {
                    Database.deleteProject(projectDelegate.projectId)
                    projectsModel.remove(projectDelegate.index, 1)
                }

                Image {
                    source: Qt.styleHints.colorScheme === Qt.Dark ? "images/close-white.png"
                                                   : "images/close.png"
                    width: 20
                    height: 20
                    anchors.centerIn: parent
                }
            }

            Column {
                id: projectContent
                topPadding: 8
                bottomPadding: 10
                leftPadding: 30

                Label {
                    text: completedTasks + " / " + totalTasks
                    font.pointSize: AppSettings.fontSize
                }

                Label {
                    id: project
                    text: projectName
                    font.pointSize: AppSettings.fontSize
                    font.styleName: "Semibold"
                }
            }

            Connections {
                target: projectDelegate
                function onClicked() {
                    let project = projectsModel.get(index)
                    root.StackView.view.push("ProjectPage.qml", {
                        "projectsModel": projectsModel,
                        "projectId": project.projectId,
                        "projectName": project.projectName,
                        "projectIndex": index,
                        "projectNote": project.projectNote,
                        "completedTasks": project.completedTasks,
                        "totalTasks": project.totalTasks
                    })
                }
            }
        }
    }

    Popup {
        id: newProjectPopup
        anchors.centerIn: parent
        height: 200
        modal: true

        ColumnLayout {
            anchors.fill: parent

            Label {
                text: qsTr("Project Name")
                font.pointSize: AppSettings.fontSize + 2.0
                font.bold: true

                Layout.alignment: Qt.AlignHCenter
            }

            TextField {
                id: newProjectTextField
                placeholderText: qsTr("Enter project name")
                font.pointSize: AppSettings.fontSize

                Layout.fillWidth: true
                Layout.leftMargin: 10
                Layout.rightMargin: 10

                Keys.onReturnPressed: {
                    if (createProjectButton.enabled)
                        createProjectButton.clicked()
                }
            }

            Button {
                id: createProjectButton
                text: qsTr("Create project")
                flat: true
                font.pointSize: AppSettings.fontSize
                enabled: newProjectTextField.length > 0

                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: false

                onClicked: {
                    let results = Database.newProject(newProjectTextField.text)
                    projectsModel.append({
                        projectId: parseInt(results.insertId),
                        projectName: newProjectTextField.text,
                        totalTasks: 0,
                        completedTasks: 0,
                        projectNote: ""
                    })
                    newProjectTextField.text = ""
                    newProjectPopup.close()
                }
            }
        }
    }

    footer: ToolBar {
        ToolButton {
            anchors.right: parent.right
            anchors.rightMargin: 5
            text: qsTr("New project")
            font.pointSize: AppSettings.fontSize - 2
            icon.source: "images/add-new.png"

            onClicked: newProjectPopup.open()
        }
    }
}
