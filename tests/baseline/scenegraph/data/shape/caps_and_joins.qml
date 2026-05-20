// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Shapes

Item {
    width: 750; height: 675

    component Shapes: Shape {
        ShapePath {
            strokeColor: "black"
            strokeWidth: 10
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            PathMove { x: 80; y: 50 }
            PathQuad { relativeX: 0; relativeY: -25;
                relativeControlX: -12.5; relativeControlY: -12.5 }
            PathQuad { relativeX: 0; relativeY: -25;
                relativeControlX: 25; relativeControlY: -12.5 }
            PathCubic { relativeX: 0; relativeY: 150;
                relativeControl1X: -125; relativeControl1Y: 50
                relativeControl2X: 125; relativeControl2Y: 100 }
            PathLine { relativeX: -75; relativeY: -125 }
            PathLine { relativeX: 37.5; relativeY: 100 }
            PathLine { relativeX: -37.5; relativeY: 0 }
        }
        ShapePath {
            strokeColor: "black"
            strokeWidth: 10
            capStyle: ShapePath.SquareCap
            joinStyle: ShapePath.MiterJoin
            PathMove { x: 80; y: 225 }
            PathQuad { relativeX: 0; relativeY: -25;
                relativeControlX: -12.5; relativeControlY: -12.5 }
            PathQuad { relativeX: 0; relativeY: -25;
                relativeControlX: 25; relativeControlY: -12.5 }
            PathCubic { relativeX: 0; relativeY: 150;
                relativeControl1X: -125; relativeControl1Y: 50
                relativeControl2X: 125; relativeControl2Y: 100 }
            PathLine { relativeX: -75; relativeY: -125 }
            PathLine { relativeX: 37.5; relativeY: 100 }
            PathLine { relativeX: -37.5; relativeY: 0 }
        }
        ShapePath {
            strokeColor: "black"
            strokeWidth: 10
            capStyle: ShapePath.FlatCap
            joinStyle: ShapePath.BevelJoin
            PathMove { x: 80; y: 400 }
            PathQuad { relativeX: 0; relativeY: -25;
                relativeControlX: -12.5; relativeControlY: -12.5 }
            PathQuad { relativeX: 0; relativeY: -25;
                relativeControlX: 25; relativeControlY: -12.5 }
            PathCubic { relativeX: 0; relativeY: 150;
                relativeControl1X: -125; relativeControl1Y: 50
                relativeControl2X: 125; relativeControl2Y: 100 }
            PathLine { relativeX: -75; relativeY: -125 }
            PathLine { relativeX: 37.5; relativeY: 100 }
            PathLine { relativeX: -37.5; relativeY: 0 }
        }

        // cosmetic stroke, scale = 1
        ShapePath {
            strokeColor: "black"
            strokeWidth: 10
            cosmeticStroke: true
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            PathMove { x: 205; y: 50 }
            PathQuad { relativeX: 0; relativeY: -25;
                relativeControlX: -12.5; relativeControlY: -12.5 }
            PathQuad { relativeX: 0; relativeY: -25;
                relativeControlX: 25; relativeControlY: -12.5 }
            PathCubic { relativeX: 0; relativeY: 150;
                relativeControl1X: -125; relativeControl1Y: 50
                relativeControl2X: 125; relativeControl2Y: 100 }
            PathLine { relativeX: -75; relativeY: -125 }
            PathLine { relativeX: 37.5; relativeY: 100 }
            PathLine { relativeX: -37.5; relativeY: 0 }
        }
        ShapePath {
            strokeColor: "black"
            strokeWidth: 10
            cosmeticStroke: true
            capStyle: ShapePath.SquareCap
            joinStyle: ShapePath.MiterJoin
            PathMove { x: 205; y: 225 }
            PathQuad { relativeX: 0; relativeY: -25;
                relativeControlX: -12.5; relativeControlY: -12.5 }
            PathQuad { relativeX: 0; relativeY: -25;
                relativeControlX: 25; relativeControlY: -12.5 }
            PathCubic { relativeX: 0; relativeY: 150;
                relativeControl1X: -125; relativeControl1Y: 50
                relativeControl2X: 125; relativeControl2Y: 100 }
            PathLine { relativeX: -75; relativeY: -125 }
            PathLine { relativeX: 37.5; relativeY: 100 }
            PathLine { relativeX: -37.5; relativeY: 0 }
        }
        ShapePath {
            strokeColor: "black"
            strokeWidth: 10
            cosmeticStroke: true
            capStyle: ShapePath.FlatCap
            joinStyle: ShapePath.BevelJoin
            PathMove { x: 205; y: 400 }
            PathQuad { relativeX: 0; relativeY: -25;
                relativeControlX: -12.5; relativeControlY: -12.5 }
            PathQuad { relativeX: 0; relativeY: -25;
                relativeControlX: 25; relativeControlY: -12.5 }
            PathCubic { relativeX: 0; relativeY: 150;
                relativeControl1X: -125; relativeControl1Y: 50
                relativeControl2X: 125; relativeControl2Y: 100 }
            PathLine { relativeX: -75; relativeY: -125 }
            PathLine { relativeX: 37.5; relativeY: 100 }
            PathLine { relativeX: -37.5; relativeY: 0 }
        }
    }
    Shapes {
        x: 50
        y: 100
        scale: 1.25
    }
    Shapes {
        x: 350
        y: 50
    }
    Shapes {
        x: 550
        y: 50
        scale: 0.5
    }
    Shapes {
        x: 550
        y: -150
        scale: 0.25
    }
}
