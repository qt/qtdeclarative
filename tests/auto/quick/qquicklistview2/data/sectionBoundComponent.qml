pragma ComponentBehavior: Bound
import QtQuick
ListView {
   id: view
   width: 100
   height: 100
   model: ListModel {
        ListElement { name: "foo"; age: 42  }
        ListElement { name: "bar"; age: 13  }
    }
    delegate: Text { required property string name; text: name}
    section.property: "age"
    section.delegate: Rectangle { color: "gray"; width: view.width; height: 20; required property string section; Text {text: parent.section} }
}
