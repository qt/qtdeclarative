import QtQuick
import QtQuick.Shapes

Rectangle {
    id: topLevel
    width: 320
    height: 480
    property bool suspendGrabbing: anim.running

    ListModel {
        id: renderers
        ListElement { renderer: Shape.GeometryRenderer }
        ListElement { renderer: Shape.CurveRenderer }
    }

    property int step: 0
    NumberAnimation on step {
        id: anim
        to: 3
        duration: 500
    }

    Row {
        padding: 10
        spacing: 20
        Repeater {
            model: renderers
            Rectangle {
                width: 140
                height: 400
                color: "lightgray"

                property int step: topLevel.step

                onStepChanged: {
                    var d;

                    // Remove first
                    d = shapes.itemAt(0).data;
                    d.splice(0, d.length).slice(1).map(x => d.push(x));

                    // Remove first, leave one
                    if (step < 3) {
                        d = shapes.itemAt(1).data;
                        d.splice(0, d.length).slice(1).map(x => d.push(x));
                    }

                    // Remove last
                    d = shapes.itemAt(2).data;
                    d.pop();

                    // Remove last, leave one
                    if (step < 3) {
                        d = shapes.itemAt(3).data;
                        d.pop();
                    }

                    // Remove middle
                    if (step === 1) {
                        d = shapes.itemAt(4).data;
                        d.splice(0, d.length).filter((p) => p.objectName != "Green2").map(x => d.push(x));
                    }

                    // Remove all
                    if (step === 1) {
                        d = shapes.itemAt(5).data;
                        d.length = 0;
                    }

                    // Remove all, then add
                    d = shapes.itemAt(6).data;
                    if (step === 1)
                        d.length = 0;
                    else if (step === 2)
                        d.push(new1);

                    // Replace one, leaving list same length
                    if (step === 1) {
                        d = shapes.itemAt(7).data;
                        d.pop();
                        d.push(new2);
                    }

                    //print("Objects left:", d.map(p => p.objectName));
                }

                ShapePath {
                    id: new1
                    objectName: "Orange1"
                    fillColor: "orange"

                    PathRectangle {
                        height: 30
                        width: 30
                        x: 0
                        y: 0
                    }
                }

                ShapePath {
                    id: new2
                    objectName: "Black1"
                    fillColor: "black"

                    PathRectangle {
                        height: 30
                        width: 30
                        x: 80
                        y: 0
                    }
                }

                Repeater {
                    id: shapes
                    model: 8

                    Shape {
                        width: parent.width
                        x: 10
                        y: 10 + 40 * index
                        preferredRendererType: renderer

                        ShapePath {
                            objectName: "Red1"
                            fillColor: "red"

                            PathRectangle {
                                height: 30
                                width: 30
                                x: 0
                                y: 0
                            }
                        }
                        ShapePath {
                            objectName: "Green2"
                            fillColor: "green"

                            PathRectangle {
                                height: 30
                                width: 30
                                radius: 5
                                x: 40
                                y: 0
                            }
                        }
                        ShapePath {
                            objectName: "Blue3"
                            fillColor: "blue"

                            PathRectangle {
                                height: 30
                                width: 30
                                radius: width / 2
                                x: 80
                                y: 0
                            }
                        }
                    }
                }
            }
        }
    }
}
