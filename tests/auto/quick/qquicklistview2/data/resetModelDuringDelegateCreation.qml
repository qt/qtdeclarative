// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Rectangle {
    id: root

    width: 240
    height: 320

    property bool didReset: false

    ListModel {
        id: listModel
    }

    // Populate after the view has been completed, so that the whole model
    // arrives as a single insertion change while the view lays out.
    Component.onCompleted: {
        for (var i = 0; i < 15; ++i)
            listModel.append({ value: i })
    }

    ListView {
        anchors.fill: parent
        objectName: "view"
        model: listModel
        cacheBuffer: 0

        delegate: Item {
            required property int value

            width: 240
            height: 20

            // Reset the model from within delegate creation, exactly once.
            // The view is iterating the insertion range at this point, so the
            // removal is delivered to MutableModelIterator re-entrantly.
            Component.onCompleted: {
                if (root.didReset)
                    return;
                root.didReset = true;
                listModel.clear();
                for (var i = 0; i < 15; ++i)
                    listModel.append({ value: i });
            }
        }
    }
}
