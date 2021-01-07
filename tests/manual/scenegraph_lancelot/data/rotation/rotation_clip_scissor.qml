import QtQuick 2.0

Rectangle {
    width: 320
    height: 480
    property int standardWidth: 60
    property int standardHeight: 60
    property int standardSpacing: 20
    property bool smoothing: true
    Rectangle {
        width: 200
        height: 200
        anchors.centerIn: parent
        color: "lightGray"
        clip: true
        Grid {
            anchors.centerIn: parent
            columns: 4
            spacing: standardSpacing
            Rectangle{ color: "red"; width: standardWidth; height: standardHeight; transform: Rotation { origin.x: standardWidth/2; origin.y: standardHeight/2 ; axis{x: 0; y: 0; z:1} angle: 5; } smooth: smoothing}
            Rectangle{ color: "orange"; width: standardWidth; height: standardHeight; transform: Rotation { origin.x: 0; origin.y: 0 ; axis{x: 0; y: 0; z:1} angle: 10; } smooth: smoothing }
            Rectangle{ color: "yellow"; width: standardWidth; height: standardHeight; transform: Rotation { origin.x: 0; origin.y: 0 ; axis{x: 0; y: 0; z:1} angle: 15; } smooth: smoothing }
            Rectangle{ color: "blue"; width: standardWidth; height: standardHeight; transform: Rotation { origin.x: 0; origin.y: 0 ; axis{x: 0; y: 0; z:1} angle: 20; } smooth: smoothing }
            Rectangle{ color: "green"; width: standardWidth; height: standardHeight; transform: Rotation { origin.x: standardWidth/2; origin.y: standardWidth/2 ; axis{x: 0; y: 0; z:1} angle: 15; } smooth: smoothing}
            Rectangle{ color: "indigo"; width: standardWidth; height: standardHeight; transform: Rotation { origin.x: 0; origin.y: 0 ; axis{x: 0; y: 0; z:1} angle: 30; } smooth: smoothing}
            Rectangle{ color: "violet"; width: standardWidth; height: standardHeight; transform: Rotation { origin.x: 0; origin.y: 0 ; axis{x: 0; y: 0; z:1} angle: 35; } smooth: smoothing }
            Rectangle{ color: "light green"; width: standardWidth; height: standardHeight; transform: Rotation { origin.x: 0; origin.y: 0 ; axis{x: 0; y: 0; z:1} angle: 40; } smooth: smoothing }
        }
    }
}
