// Copyright (C) 2022 zccrs <zccrs@live.com>, JiDe Zhang <zhangjide@uniontech.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window
    property alias control: control
    property alias item1Palette: item1.palette
    property alias item2Palette: item2.palette

    palette {
        disabled {
            windowText: "blue"
        }
    }

    Item {
        id: item1

        palette.disabled {
            text: "yellow"
        }

        Item {
            id: item2

            palette.disabled {
                window: "yellow"
            }
        }
    }

    Control {
        id: control
        objectName: "Control"

        palette {
            window: "red"
            disabled: item2.palette.disabled
        }
    }

    function resetColor() {
        control.palette.window = undefined
    }

    function resetGroup() {
        control.palette.disabled = undefined
    }
}
