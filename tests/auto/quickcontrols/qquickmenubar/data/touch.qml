// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
