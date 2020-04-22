/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
    property alias tableView: tableView

    function appendBanana() {
        testModel.appendRow({
            fruit: "Banana",
            price: 3.5
        })
    }

    function appendStrawberry() {
        testModel.appendRow({
            fruit: "Strawberry",
            price: "5"
        })
    }

    function appendInvalid() {
        testModel.appendRow({
            fruit: "Pear",
            price: "Invalid"
        })
    }

    TableModel {
        id: testModel
        objectName: "testModel"

        TableModelColumn { display: "fruit" }
        TableModelColumn { display: "price" }
        rows: [
            {
                fruit: "Apple",
                price: 1
            },
            {
                fruit: "Orange",
                price: 2
            }
        ]
    }
    TableView {
        id: tableView
        anchors.fill: parent
        model: testModel
        delegate: Text {
            text: model.display
        }
    }
}
