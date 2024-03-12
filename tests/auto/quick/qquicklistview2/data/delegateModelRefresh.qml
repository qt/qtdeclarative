// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

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
