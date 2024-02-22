// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.0
import QtTest 1.1

Item {
    id: top

    Gradient {
        id: emptygradient
    }

    Gradient {
        id: twogradients
        GradientStop { position: 0.0; color: "red" }
        GradientStop { position: 1.0; color: "green" }
    }

    Gradient {
        id: elevengradients
        GradientStop { position: 0.0; color: "red" }
        GradientStop { position: 0.1; color: "orange" }
        GradientStop { position: 0.2; color: "yellow" }
        GradientStop { position: 0.3; color: "green" }
        GradientStop { position: 0.4; color: "blue" }
        GradientStop { position: 0.5; color: "violet" }
        GradientStop { position: 0.6; color: "indigo" }
        GradientStop { position: 0.7; color: "brown" }
        GradientStop { position: 0.8; color: "lightgray" }
        GradientStop { position: 0.9; color: "gray" }
        GradientStop { position: 1.0; color: "black" }
    }

    Gradient {
        id: movedgradients
        property real stopposition: 0.5
        GradientStop { position: 0.0; color: "red" }
        GradientStop { position: movedgradients.stopposition; color: "blue" }
        GradientStop { position: 1.0; color: "green" }
    }

    Gradient {
        id: defaultgradient
        GradientStop { }
        GradientStop { position: 1.0; color: "red" }
    }

    TestCase {
        name: "Gradient"

        function test_empty() {
            compare(emptygradient.stops.length, 0)
        }

        function test_lengthtwo() {
            compare(twogradients.stops.length, 2)
            compare(twogradients.stops[0].color.toString(), "#ff0000")
            compare(twogradients.stops[1].color.toString(), "#008000")
        }

        function test_multiplestops() {
            compare(elevengradients.stops.length, 11)
            compare(elevengradients.stops[0].color.toString(), "#ff0000")
            compare(elevengradients.stops[4].color.toString(), "#0000ff")
            compare(elevengradients.stops[4].position, 0.4)
            compare(elevengradients.stops[9].position, 0.9)
        }

        function test_moved() {
            compare(movedgradients.stops.length, 3)
            compare(movedgradients.stops[1].position, 0.5)
            movedgradients.stopposition = 0.3;
            compare(movedgradients.stops[1].position, 0.3)
        }

        function test_default() {
            compare(defaultgradient.stops.length, 2)
            compare(defaultgradient.stops[0].color.toString(), "#000000")
        }
    }
}
