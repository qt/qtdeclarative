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
import QtQuick.Controls
import QtQml.Models

ApplicationWindow {
    id: window
    visible: true
    width: 400
    height: 800
    property bool done: false

    ListView {
        model: delegateModel
        anchors.fill: parent
    }

    DelegateModel {
        id: delegateModel
        model: ListModel {
            ListElement {
                available: true
            }
            ListElement {
                available: true
            }
            ListElement {
                available: true
            }
        }

        Component.onCompleted: {
            delegateModel.refresh()
            done = true;
        }
        function refresh() {
            var rowCount = delegateModel.model.count;
            const flatItemsList = []
            for (var i = 0; i < rowCount; i++) {
                var entry = delegateModel.model.get(i);
                flatItemsList.push(entry)
            }

            for (i = 0; i < flatItemsList.length; ++i) {
                var item = flatItemsList[i]
                if (item !== null)
                    items.insert(item)
            }
        }
    }
}
