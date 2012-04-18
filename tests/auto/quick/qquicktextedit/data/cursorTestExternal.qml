import QtQuick 2.0

Rectangle { width: 300; height: 300; color: "white"
    property string contextualProperty: "Hello"
    TextEdit {
        text: "Hello world!"
        id: textEditObject;
        objectName: "textEditObject"
        wrapMode: TextEdit.WordWrap
        cursorDelegate: Cursor {
            id:cursorInstance;
            objectName: "cursorInstance";
            localProperty: contextualProperty;
        }
    }
}
