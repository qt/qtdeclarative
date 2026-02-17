// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import Qt.labs.StyleKit

ApplicationWindow {
    id: app
    width: 1024
    height: 800
    visible: true

    StyleKit.style:
    //! [ControlStyle]
    Style {
        control { // ControlStyle
            padding: 6
            background { // DelegateStyle
                color: "slategray"
            }
            indicator { // DelegateStyle
                foreground.color: "slateblue"
            }
            handle { // DelegateStyle
                radius: 10
            }
        }

        button { // ControlStyle
        }

        slider { // ControlStyle
        }
    }
    //! [ControlStyle]
}
