/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

import QtQuick 2.12
import Qt.labs.qmlmodels 1.0

Item {
    id: root
    width: 200
    height: 200

    property alias testModel: testModel

    signal shouldModify()
    signal shouldModifyInvalidRole()
    signal shouldModifyInvalidType()

    function modify() {
        shouldModify()
    }

    function modifyInvalidRole() {
        shouldModifyInvalidRole()
    }

    function modifyInvalidType() {
        shouldModifyInvalidType()
    }

    TableView {
        anchors.fill: parent
        model: TestModel {
            id: testModel
        }

        delegate: Text {
            id: textItem
            text: model.display

            Connections {
                target: root
                enabled: column === 1
                onShouldModify: model.display = 18
            }

            Connections {
                target: root
                enabled: column === 0
                // Invalid: should be "display".
                onShouldModifyInvalidRole: model.age = 100
            }

            Connections {
                target: root
                enabled: column === 1
                // Invalid: should be string.
                onShouldModifyInvalidType: model.display = "Whoops"
            }
        }
    }
}
