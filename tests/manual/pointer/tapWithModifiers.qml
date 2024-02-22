// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.12

Item {
    width: 200
    height: 200
    TapHandler {
        acceptedModifiers: Qt.ControlModifier
        onTapped: console.log("control-tapped")
    }
    TapHandler {
        acceptedModifiers: Qt.NoModifier
        onTapped: console.log("tapped with no modifiers")
    }
    TapHandler {
        onTapped:
            switch (point.modifiers) {
            case Qt.ControlModifier | Qt.AltModifier:
                console.log("CTRL+ALT");
                break;
            case Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier:
                console.log("CTRL+META+ALT");
                break;
            default:
                console.log("other modifiers", point.modifiers)
            }
    }
}
