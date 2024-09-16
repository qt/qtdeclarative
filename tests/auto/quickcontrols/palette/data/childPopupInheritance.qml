// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: window

    Popup {
        id: parentPopup
        objectName: "parentPopup"
        palette.windowText: "#ffdead"

        Label {
            text: "parentPopupLabel"
            objectName: text
        }

        Popup {
            id: childPopup
            objectName: "childPopup"

            Label {
                text: "childPopupLabel"
                objectName: text
            }

            Popup {
                id: grandchildPopup
                objectName: "grandchildPopup"

                Label {
                    text: "grandchildPopupLabel"
                    objectName: text
                }
            }
        }
    }
}
