import QtQuick 2.0

Rectangle { id:rect; width: 300; height: 300; color: "white"
    property string contextualProperty: "Hello"
    TextInput {  text: "Hello world!"; id: textInputObject; objectName: "textInputObject"
        resources: [ Component { id:cursor; Item { id:cursorInstance; objectName: "cursorInstance"; property string localProperty: contextualProperty } } ]
        cursorDelegate: cursor
    }
}
