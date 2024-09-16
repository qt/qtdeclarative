// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window

    property alias control: control
    property alias child: child
    property alias grandChild: grandChild

    Control {
        id: control

        Control {
            id: child

            Item {
                Control {
                    id: grandChild
                }
            }
        }
    }
}
