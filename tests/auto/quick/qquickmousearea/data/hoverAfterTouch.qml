import QtQuick 2.0

Item {
    width: 200
    height: 260

    // The bottom 60px are deliberately left empty, so that the tests have somewhere to
    // park the mouse cursor that is outside both MouseAreas.
    MouseArea {
        objectName: "upperArea"
        y: 0
        width: 200; height: 100
        hoverEnabled: true
    }

    MouseArea {
        objectName: "lowerArea"
        y: 100
        width: 200; height: 100
        hoverEnabled: true
    }
}
