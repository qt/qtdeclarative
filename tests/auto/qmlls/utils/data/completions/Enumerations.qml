// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick

Item {
    enum Hello { Kitty, World }

    Item {
        Item {
            Item {
                property int i: Enumerations.Hello.World
                property int j: Enumerations.World
            }
        }
    }

    property int i: Enumerations.Hello.World
    property int j: Enumerations.World

}
