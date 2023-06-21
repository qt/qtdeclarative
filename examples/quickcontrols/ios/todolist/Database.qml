// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

pragma Singleton

import QtQml
import QtQuick.LocalStorage

QtObject {
    id: root

    property var _db

    function _database() {
        if (_db)
            return _db

        try {
            let db = LocalStorage.openDatabaseSync("ToDoList", "1.0", "ToDoList application database")

            db.transaction(function (tx) {
                tx.executeSql(`CREATE TABLE IF NOT EXISTS projects (
                    project_id INTEGER PRIMARY KEY AUTOINCREMENT,
                    project_name TEXT NOT NULL CHECK(project_name != ''),
                    project_note TEXT
                )`);
            })

            db.transaction(function (tx) {
                tx.executeSql(`CREATE TABLE IF NOT EXISTS tasks (
                    task_id INTEGER PRIMARY KEY AUTOINCREMENT,
                    task_name TEXT CHECK(task_name != ''),
                    done INTEGER,
                    project_id,
                    FOREIGN KEY(project_id) REFERENCES projects(project_id)
                )`);
            })

            _db = db
        } catch (error) {
            console.log("Error opening database: " + error)
        };

        return _db
    }

    function getProjects() {
        let projects = []
        root._database().transaction(function (tx) {
            let results = tx.executeSql('SELECT * FROM projects')
            for (let i = 0; i < results.rows.length; i++) {
                let projectRow = results.rows.item(i)
                let projectId = projectRow.project_id
                let completedTasks = Math.max(countDoneTasksByProject(projectId).rows.length, 0)
                let totalTasks = Math.max(countTaskByProject(projectId).rows.length, 0)
                projects.push({
                    "projectName": projectRow.project_name,
                    "projectId": projectId,
                    "projectNote": projectRow.project_note ?? "",
                    "completedTasks": completedTasks,
                    "totalTasks": totalTasks
                })
            }
        })
        return projects
    }

    function newProject(projectName) {
        let results
        root._database().transaction(function (tx) {
            results = tx.executeSql("INSERT INTO projects (project_name) VALUES(?)", [projectName])
        })
        return results
    }

    function updateProjectNote(projectId, projectNote) {
        root._database().transaction(function (tx) {
            tx.executeSql("UPDATE projects set project_note=? WHERE project_id=?", [projectNote, projectId])
        })
    }

    function updateProjectName(projectId, projectName) {
        root._database().transaction(function (tx) {
            tx.executeSql("UPDATE projects set project_name=? WHERE project_id=?", [projectName, projectId])
        })
    }

    function deleteProject(projectId) {
        root._database().transaction(function (tx) {
            deleteAllTasks(projectId)
            tx.executeSql("DELETE FROM projects WHERE project_id = ?", [projectId])
        })
    }

    function getTaskByProject(projectId) {
        if (!projectId)
            return

        let tasks = []
        root._database().transaction(function (tx) {
            let results = tx.executeSql("SELECT * FROM tasks WHERE project_id = " + [projectId] + " ORDER BY done")
            for (let i = 0; i < results.rows.length; i++) {
                let row = results.rows.item(i)
                tasks.push({
                    "taskId": row.task_id,
                    "taskName": row.task_name,
                    "done": row.done === 1 ? true : false
                })
            }
        })
        return tasks
    }

    function countTaskByProject(projectId) {
        let results
        root._database().transaction(function (tx) {
            results = tx.executeSql('SELECT task_id FROM tasks WHERE project_id =' + [projectId])
        })
        return results
    }

    function countDoneTasksByProject(projectId) {
        let results
        root._database().transaction(function (tx) {
            results = tx.executeSql("SELECT task_id FROM tasks WHERE project_id =" + [projectId] + " AND done = 1")
        })
        return results
    }

    function newTask(projectId, taskName) {
        let results
        root._database().transaction(function (tx) {
            results = tx.executeSql("INSERT INTO tasks (task_name, done, project_id) VALUES(?, 0, ?)", [taskName, projectId])
        })
        return results
    }

    function updateTaskName(taskId, taskName) {
        root._database().transaction(function (tx) {
            tx.executeSql("UPDATE tasks set task_name=? WHERE task_id=?", [taskName, taskId])
        })
    }

    function updateDoneState(taskId, doneState) {
        root._database().transaction(function (tx) {
            tx.executeSql("UPDATE tasks set done=? WHERE task_id=?", [doneState, taskId])
        })
    }

    function deleteAllTasks(projectId) {
        root._database().transaction(function (tx) {
            tx.executeSql("DELETE FROM tasks WHERE project_id =" + projectId)
        })
    }

    function deleteTask(taskId) {
        root._database().transaction(function (tx) {
            tx.executeSql("DELETE FROM tasks WHERE task_id = ?", [taskId])
        })
    }
}
