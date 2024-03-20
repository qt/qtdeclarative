import QtQuick 2.15
import QtQuick.Controls 2.15

QtObject {
    id: root

    property int currentCategory: 0



    property var materialsNames: [[qsTr("Car Paint"), qsTr("Glittery Car Paint"), qsTr("Aluminium"), qsTr("Chrome"), qsTr("Steel"), qsTr("Brushed Steel"), qsTr("Steel Floor"), qsTr("Copper"), qsTr("Silver"), qsTr("Gold"), qsTr("Mirror")],
                                  [qsTr("Asphalt"), qsTr("Brick"), qsTr("Ceramic"), qsTr("Concrete"), qsTr("Glass"), qsTr("Darkened Glass"), qsTr("Wood"), qsTr("Wood - Planks"), qsTr("Wood - Parquet"), qsTr("Stone")],
                                  [qsTr("Acrylic Paint"), qsTr("Carbon Fiber"), qsTr("Shiny Plastic"), qsTr("Matte Plastic"), qsTr("Textured Plastic"), qsTr("Rubber"), qsTr("Wax")],
                                  [qsTr("Fabric"), qsTr("Fabric: Rough"), qsTr("Fabric: Satin"), qsTr("Leather"), qsTr("Paper")]]



   property var model: root.materialsNames[root.currentCategory]

}