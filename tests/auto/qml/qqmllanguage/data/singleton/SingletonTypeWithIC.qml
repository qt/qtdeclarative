import QtQuick 2.0
pragma Singleton

Item {
     id: singletonId
     component IC1: Item {
         property int iProp: 42
         property string sProp: "Hello, world"
         property Rectangle myRect: Rectangle {color: "green"}
     }
     component IC2: Item {
         property int iProp: 13
         property string sProp: "Goodbye, world"
         property Rectangle myRect: Rectangle {color: "red"}
     }
}
