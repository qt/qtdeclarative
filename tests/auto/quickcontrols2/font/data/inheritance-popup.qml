// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window

    property alias control: control
    property alias child: child
    property alias grandChild: grandChild

    Popup {
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
