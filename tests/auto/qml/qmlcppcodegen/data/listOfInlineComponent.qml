import QtQml

QtObject {
    id: root

    // NOTE: No crash if:
    // - We use list<QtObject> instead of list<QMLType>, OR
    // - We set NO_CACHEGEN in qt_add_qml_module(), OR
    // - We use Windows instead of macOS, OR
    // - We swap the two lines below
    property list<QMLType> qmlTypeList
    component QMLType: QtObject {}

    property var model: root.qmlTypeList
}
