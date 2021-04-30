import QtQuick 2.0

ListModel {
    id: listModel
    ListElement { dataType: "rect"; color: "green"; font.color: "red"; ListElement {} Behavior on FooBar {} }
    ListElement { dataType: "image"; source: "../shared/images/qt-logo.png" }
    ListElement { dataType: "rect"; color: "green" }
    ListElement { dataType: "image"; source: "../shared/images/qt-logo.png" }
    ListElement { dataType: "rect"; color: "blue" }
    ListElement { dataType: "rect"; color: "blue" }
    ListElement { dataType: "rect"; color: "blue" }
    ListElement { dataType: "rect"; color: "blue" }
    ListElement { dataType: "rect"; color: "blue" }
    ListElement { dataType: "rect"; color: "blue" }
}

