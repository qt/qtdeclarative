import QtQuick

ListView {
   model: ListModel {}
   delegate: Text {
       text: model.text
       color: index % 2 ? "red" : "blue"
   }
}
