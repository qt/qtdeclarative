// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    id: rootId
    property int inRoot

    DelegateModel {
        delegate: Item {
            id: childId

            property int myInt: rootId.inRoot
            property int inChild

        }
    }

    property int myInt: childId.inChild
}
