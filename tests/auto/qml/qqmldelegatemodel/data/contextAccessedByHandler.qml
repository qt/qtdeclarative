import QtQuick 2.0

Item {
    id: root
    width: 640
    height: 480
    property bool works: myView.currentItem.okay

    ListView {
        id: myView
        model: myModel
        anchors.fill: parent
        delegate: Row {
            property alias okay: image.isSelected
            ImageToggle {
                id: image
                source: "glyph_16_arrow_patch"
                isSelected: model.age < 6
            }
            Text {
                text: "age:" + model.age + " selected:" + image.isSelected
            }
        }
    }

    ListModel {
        id: myModel
        ListElement { type: "Cat"; age: 3; }
        ListElement { type: "Dog"; age: 2; }
    }
}
