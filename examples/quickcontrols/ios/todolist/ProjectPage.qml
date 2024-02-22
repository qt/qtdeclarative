// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Page {
    id: root

    required property int projectIndex
    required property int projectId
    required property string projectName
    required property string projectNote

    required property int completedTasks
    required property int totalTasks

    required property ListModel projectsModel

    header: ColumnLayout {
        id: titleRow
        spacing: 4

        TextField {
            id: projectNameLabel
            text: root.projectName
            font.pointSize: AppSettings.fontSize + 10
            font.styleName: "Bold"
            padding: 0
            background: null
            wrapMode: TextField.Wrap

            Layout.topMargin: 10
            Layout.leftMargin: 20
            Layout.rightMargin: 20

            onEditingFinished: {
                Database.updateProjectName(root.projectId, text)
                projectsModel.setProperty(projectIndex, "projectName", projectNameLabel.text)
            }
        }

        Label {
            text: root.completedTasks + " / " + root.totalTasks
            Layout.leftMargin: 20
        }

        ProgressBar {
            id: progressBar
            from: 0
            value: root.completedTasks
            to: root.totalTasks
            Layout.leftMargin: 20
            Layout.fillWidth: false
        }
    }

    ColumnLayout {
        anchors.fill: parent

        TextArea {
            id: textArea
            leftPadding: 15
            rightPadding: doneButton.width
            bottomPadding: 10
            topPadding: 10
            font.pointSize: AppSettings.fontSize
            placeholderText: qsTr("Write a note...")
            text: root.projectNote
            wrapMode: TextArea.Wrap
            clip: true

            Layout.preferredHeight: 80
            Layout.topMargin: 20

            onEditingFinished: {
                Database.updateProjectNote(root.projectId, text)
                projectsModel.setProperty(projectIndex, "projectNote", textArea.text)
            }

            Button {
                id: doneButton
                text: qsTr("Done")
                flat: true
                visible: textArea.focus
                onClicked: textArea.editingFinished()
                anchors.right: parent.right
                anchors.bottom: parent.bottom
            }
        }

        ListView {
            id: taskListView
            model: taskModel
            clip: true

            Layout.topMargin: 20

            ListModel {
                id: taskModel

                Component.onCompleted: {
                    let tasks = Database.getTaskByProject(projectId)
                    for (let task of tasks)
                        append(task)
                }
            }

            delegate: CheckDelegate {
                id: taskList
                width: taskListView.width
                height: visible ? 40 : 0
                checked: done
                visible: !done || (done && AppSettings.showDoneTasks)

                required property bool done
                required property int taskId
                required property string taskName
                required property int index

                onClicked: {
                    Database.updateDoneState(taskId, checked ? 1 : 0)
                    taskModel.setProperty(index, "done", checked)
                    root.completedTasks = Math.max(Database.countDoneTasksByProject(root.projectId).rows.length, 0)
                    root.totalTasks = Math.max(Database.countTaskByProject(root.projectId).rows.length, 0)
                    root.projectsModel.setProperty(projectIndex, "completedTasks", root.completedTasks)
                }

                TextField {
                    id: taskNameLabel
                    text: taskList.taskName
                    anchors.left: deleteTaskButton.right
                    anchors.leftMargin: 10
                    padding: 0
                    font.pointSize: AppSettings.fontSize
                    anchors.verticalCenter: parent.verticalCenter
                    background: null

                    onEditingFinished: {
                        Database.updateTaskName(taskList.taskId, taskNameLabel.text)
                        taskModel.setProperty(index, "taskName", taskNameLabel.text)
                    }
                }

                Button {
                    id: deleteTaskButton
                    anchors.left: parent.left
                    width: 15
                    height: 15
                    flat: true
                    topPadding: 0
                    bottomPadding: 0
                    rightPadding: 0
                    leftPadding: 0
                    anchors.leftMargin: 10
                    anchors.verticalCenter: parent.verticalCenter
                    icon.source: "images/close.png"
                    icon.color: Qt.styleHints.colorScheme === Qt.Dark ? "white" : "black"

                    onClicked: {
                        Database.deleteTask(taskList.taskId)
                        taskModel.remove(index, 1)
                        root.totalTasks--
                        if (taskList.done)
                            root.completedTasks--
                    }
                }
            }
        }

        Popup {
            id: addTaskPopup
            parent: root
            anchors.centerIn: parent
            height: 200
            modal: true
            focus: true

            ColumnLayout {
                anchors.fill: parent

                Label {
                    text: qsTr("Add New Task")
                    font.pointSize: AppSettings.fontSize + 2.0
                    font.bold: true

                    Layout.alignment: Qt.AlignHCenter
                }

                TextField {
                    id: newTaskNameTextField
                    placeholderText: qsTr("Enter task name")
                    font.pointSize: AppSettings.fontSize

                    Keys.onReturnPressed: {
                        if (addTaskButton.enabled)
                            addTaskButton.clicked()
                    }

                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    Layout.alignment: Qt.AlignHCenter
                }

                Button {
                    id: addTaskButton
                    text: qsTr("Add task")
                    flat: true
                    font.pointSize: AppSettings.fontSize
                    enabled: newTaskNameTextField.length > 0

                    Layout.alignment: Qt.AlignHCenter

                    onClicked: {
                        const result = Database.newTask(root.projectId, newTaskNameTextField.text)
                        taskModel.append({
                            "taskId": parseInt(result.insertId),
                            "taskName": newTaskNameTextField.text, "done": false
                        })
                        root.projectsModel.setProperty(projectIndex, "totalTasks", taskListView.count)
                        newTaskNameTextField.text = ""
                        root.totalTasks++
                        addTaskPopup.close()
                    }
                }
            }
        }

        Label {
            text: qsTr("Task limit reached.")
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 20
            font.pointSize: AppSettings.fontSize
            visible: taskListView.count >= AppSettings.maxTasks
        }
    }

    footer: ToolBar {
        ToolButton {
            anchors.right: parent.right
            anchors.rightMargin: 5
            text: qsTr("New task")
            font.pointSize: AppSettings.fontSize - 2
            icon.source: "images/add-new.png"
            enabled: taskListView.count < AppSettings.maxTasks

            onClicked: addTaskPopup.open()
        }
    }
}
