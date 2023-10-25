import QtQuick

Item {
    property QtObject qtobjectParent: QtObject { }
    property QtObject itemParent: Item { }
    property QtObject windowParent: Window { }

    property QtObject qtobject_qtobject : null
    property QtObject qtobject_item : null
    property QtObject qtobject_window : null

    property QtObject item_qtobject : null
    property QtObject item_item : null
    property QtObject item_window : null

    property QtObject window_qtobject : null
    property QtObject window_item : null
    property QtObject window_window : null

    Component.onCompleted: {
        qtobject_qtobject = Qt.createQmlObject("import QtQuick; QtObject{}", qtobjectParent);
        qtobject_item = Qt.createQmlObject("import QtQuick; Item{}", qtobjectParent);
        qtobject_window = Qt.createQmlObject("import QtQuick; Window{}", qtobjectParent);
        item_qtobject = Qt.createQmlObject("import QtQuick; QtObject{}", itemParent);
        item_item = Qt.createQmlObject("import QtQuick; Item{}", itemParent);
        item_window = Qt.createQmlObject("import QtQuick; Window{}", itemParent);
        window_qtobject = Qt.createQmlObject("import QtQuick; QtObject{}", windowParent);
        window_item = Qt.createQmlObject("import QtQuick; Item{}", windowParent);
        window_window = Qt.createQmlObject("import QtQuick; Window{}", windowParent);
    }
}
