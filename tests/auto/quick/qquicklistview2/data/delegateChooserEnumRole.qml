// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick
import Qt.labs.qmlmodels
import Test

ListView {
    width: 300; height: 300
    model: TypeRoleModel {}
    delegate: DelegateChooser {
        role: "type"
        DelegateChoice {
            roleValue: 0
            Text {
                property int delegateType: 0
                text: model.text + " of type " + model.type
            }
        }
        DelegateChoice {
            roleValue: "Markdown"
            Text {
                property int delegateType: 1
                text: model.text + " of **type** " + model.type
                textFormat: Text.MarkdownText
            }
        }
        DelegateChoice {
            roleValue: TypeRoleModel.Rect
            Rectangle {
                property int delegateType: 2
                width: 300; height: 20
                color: "wheat"
                Text {
                    text: model.text + " of type " + model.type
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }
}
