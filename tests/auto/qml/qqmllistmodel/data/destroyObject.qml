import QtQml

QtObject {
    id: root
    property ListModel projects: ListModel {}
    property var object

    Component.onCompleted: {
        var comp= Qt.createComponent("dummyItem.qml");
        object = comp.createObject(root, {});
        projects.append({"obj": object});
    }

    function destroy() {
        try {
            object.destroy();
        } catch(e) {
            console.warn(e);
            return false;
        }
        return true;
    }
}
