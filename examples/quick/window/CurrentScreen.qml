// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

import QtQuick
import QtQuick.Controls

Item {
    id: root
    width: 400
    height: propertyGrid.implicitHeight + 16

    function orientationToString(o) {
        switch (o) {
        case Qt.PrimaryOrientation:
            return "primary";
        case Qt.PortraitOrientation:
            return "portrait";
        case Qt.LandscapeOrientation:
            return "landscape";
        case Qt.InvertedPortraitOrientation:
            return "inverted portrait";
        case Qt.InvertedLandscapeOrientation:
            return "inverted landscape";
        }
        return "unknown";
    }

    Grid {
        id: propertyGrid
        columns: 2
        spacing: 8
        x: spacing
        y: spacing

        //! [screen]
        Label {
            text: "Screen \"" + Screen.name + "\":"
            font.bold: true
        }
        Item { width: 1; height: 1 } // spacer

        Label { text: "manufacturer" }
        Label { text: Screen.manufacturer ? Screen.manufacturer : "unknown" }

        Label { text: "model" }
        Label { text: Screen.model ? Screen.model : "unknown" }

        Label { text: "serial number" }
        Label { text: Screen.serialNumber ? Screen.serialNumber : "unknown" }

        Label { text: "dimensions" }
        Label { text: Screen.width + "x" + Screen.height }

        Label { text: "pixel density" }
        Label { text: Screen.pixelDensity.toFixed(2) + " dots/mm (" + (Screen.pixelDensity * 25.4).toFixed(2) + " dots/inch)" }

        Label { text: "logical pixel density" }
        Label { text: Screen.logicalPixelDensity.toFixed(2) + " dots/mm (" + (Screen.logicalPixelDensity * 25.4).toFixed(2) + " dots/inch)" }

        Label { text: "device pixel ratio" }
        Label { text: Screen.devicePixelRatio.toFixed(2) }

        Label { text: "available virtual desktop" }
        Label { text: Screen.desktopAvailableWidth + "x" + Screen.desktopAvailableHeight }

        Label { text: "position in virtual desktop" }
        Label { text: Screen.virtualX + ", " + Screen.virtualY }

        Label { text: "orientation" }
        Label { text: root.orientationToString(Screen.orientation) + " (" + Screen.orientation + ")" }

        Label { text: "primary orientation" }
        Label { text: root.orientationToString(Screen.primaryOrientation) + " (" + Screen.primaryOrientation + ")" }
        //! [screen]

        Label { text: "10mm rectangle" }
        Rectangle {
            color: "red"
            width: Screen.pixelDensity * 10
            height: width
        }
    }
}
