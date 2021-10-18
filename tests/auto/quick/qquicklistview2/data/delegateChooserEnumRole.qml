/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
