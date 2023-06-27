// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
//![0]
import QtQuick
import QtQml.Models

ListView {
    id: view
    width: 300
    height: 400

    model: DelegateModel {
        model: fileSystemModel

        delegate: Rectangle {
            width: 200; height: 25
            Text { text: filePath }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (model.hasModelChildren)
                        view.model.rootIndex = view.model.modelIndex(index)
                }
            }
        }
    }
}
//![0]
