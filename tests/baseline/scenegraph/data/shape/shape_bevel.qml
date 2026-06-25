// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

import QtQuick
import QtQuick.Shapes

Rectangle {
    width: 320
    height: 480
    color: "lightgray"

    ListModel {
        id: renderers
        ListElement { renderer: Shape.GeometryRenderer }
        ListElement { renderer: Shape.CurveRenderer }
    }

    Row {
        padding: 10
        Repeater {
            model: renderers
            Column {
                spacing: 10
                Shape {
                    width: 160
                    preferredRendererType: renderer

                    ShapePath {
                        fillColor: "transparent"
                        strokeColor: "blue"
                        strokeWidth: 1

                        PathRectangle {
                            x: 20; y: 0
                            width: 100; height: 20
                            radius: 8
                            bevel: true
                        }

                        PathRectangle {
                            x: 20.5; y: 30.5
                            width: 100; height: 20
                            radius: 8
                            bevel: true
                        }
                    }
                }

                Shape {
                    width: 160
                    preferredRendererType: renderer

                    ShapePath {
                        fillColor: "yellow"
                        strokeColor: "transparent"

                        PathRectangle {
                            x: 20; y: 0
                            width: 100; height: 20
                            radius: 8
                            bevel: true
                        }

                        PathRectangle {
                            x: 20.5; y: 30.5
                            width: 100; height: 20
                            radius: 8
                            bevel: true
                        }
                    }
                }

                Shape {
                    width: 160
                    preferredRendererType: renderer

                    ShapePath {
                        fillColor: "yellow"
                        strokeColor: "green"
                        strokeWidth: 5
                        joinStyle: ShapePath.RoundJoin

                        PathRectangle {
                            x: 20; y: 00
                            width: 100; height: 20
                            radius: 8
                            bevel: true
                        }

                        PathRectangle {
                            x: 20; y: 30
                            width: 100; height: 20
                            radius: 5
                            bevel: true
                        }
                    }

                    ShapePath {
                        fillColor: "yellow"
                        strokeColor: "green"
                        strokeWidth: 5
                        joinStyle: ShapePath.MiterJoin

                        PathRectangle {
                            x: 20; y: 60
                            width: 100; height: 20
                            radius: 8
                            bevel: true
                        }

                        PathRectangle {
                            x: 20; y: 90
                            width: 100; height: 20
                            radius: 5
                            bevel: true
                        }

                        PathRectangle {
                            x: 20; y: 120
                            width: 100; height: 20
                            radius: 50
                            bevel: true
                        }

                        PathRectangle {
                            x: 20; y: 150
                            width: 100; height: 30
                            radius: 10
                            topLeftRadius: 50
                            bottomRightRadius: 5
                            bottomLeftRadius: 0
                            topLeftBevel: true
                            bottomRightBevel: true
                        }
                    }
                }

                Shape {
                    width: 160
                    preferredRendererType: renderer

                    ShapePath {
                        id: myPath
                        fillColor: "white"
                        strokeColor: "blue"
                        strokeWidth: 20
                        joinStyle: ShapePath.MiterJoin

                        PathRectangle {
                            width: 120
                            height: 60
                            topRightRadius: 30
                            bevel: true
                            strokeAdjustment: myPath.strokeWidth
                        }
                    }
                }
            }
        }
    }
}
