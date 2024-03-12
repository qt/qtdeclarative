// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick 2.12
import QtTest 1.1

Item {
    id: top

    Rectangle { id: empty }

    Rectangle { id: radius }

    Rectangle {
        id: resized
        width: 300
        height: 300
    }

    Rectangle {
        id: smooth
        smooth: true
        width: 300
        height: 300
    }

    Rectangle {
        id: gradient
        width: 100
        height: 300
        gradient: Gradient {
            GradientStop { position: 0.0; color: "red" }
            GradientStop { position: 0.5; color: "yellow" }
            GradientStop { position: 1.0; color: "green" }
        }
    }

    Rectangle {
        id: horizontalGradient
        width: 300
        height: 100
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: "red" }
            GradientStop { position: 1.0; color: "green" }
        }
    }

    Rectangle {
        id: rectangleborder
        width: 300
        height: 150
        border.width: 1
        border.color: "gray"
    }

    TestCase {
        name: "Rectangle"

        function test_empty() {
            compare(empty.width, 0)
            compare(empty.height, 0)
        }

        function test_radius() {
            compare(radius.width, 0)
            compare(radius.height, 0)
            compare(radius.radius, 0)
            radius.height = 100;
            radius.width = 100;
            radius.radius = 10;
            compare(radius.width, 100)
            compare(radius.height, 100)
            compare(radius.radius, 10)
        }

        function test_resized() {
            compare(resized.width, 300)
            compare(resized.height, 300)
            resized.height = 500;
            resized.width = 500;
            compare(resized.width, 500)
            compare(resized.height, 500)
        }

        function test_smooth() {
            compare(smooth.smooth, true)
            compare(smooth.width, 300)
            compare(smooth.height, 300)

        }

        function test_gradient() {
            var grad = gradient.gradient;
            var gstops = grad.stops;
            compare(gstops[0].color.toString(), "#ff0000")
            compare(gstops[1].color.toString(), "#ffff00")
            compare(gstops[2].color.toString(), "#008000")
            compare(grad.orientation, Gradient.Vertical)
        }

        function test_horizontalGradient() {
            var grad = horizontalGradient.gradient;
            var gstops = grad.stops;
            compare(gstops[0].color.toString(), "#ff0000")
            compare(gstops[1].color.toString(), "#008000")
            compare(grad.orientation, Gradient.Horizontal)
        }


        function test_borders() {
            compare(rectangleborder.border.width, 1)
            compare(rectangleborder.border.color.toString(), "#808080")
            rectangleborder.border.width = 10;
            rectangleborder.border.color = "brown";
            compare(rectangleborder.border.width, 10)
            compare(rectangleborder.border.color.toString(), "#a52a2a")
        }

    }
}
