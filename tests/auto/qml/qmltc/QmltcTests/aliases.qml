// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import QtQuick

Item {
    property alias aliasToAlias: subItem.aliasToAlias
    Item {
        id: subItem
        property alias aliasToAlias: subsubItem.aliasToAlias

        Item {
            id: subsubItem
            property alias aliasToAlias: subsubsubItem.value

                Item {
                id: subsubsubItem
                property string value: "Hello World!"
            }
        }
    }

    property alias aliasToOtherFile: inOtherFile.setMe

    ComponentWithAlias1 {
        id: inOtherFile
    }
}

