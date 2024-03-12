// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    width: 400
    height: 400

    header: MenuBar {
        Menu {
            title: "&File"

            MenuItem {
                text: "&Open..."
            }
        }
    }
}
