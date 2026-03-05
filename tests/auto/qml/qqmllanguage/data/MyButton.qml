import QtQml

QtObject {
    property QtObject action
    property alias text: labelWithOptionMenu.label.property
    property MyLabel l: MyLabel {
        id: labelWithOptionMenu
        label.property: action ? action.objectName : ""
    }
}
