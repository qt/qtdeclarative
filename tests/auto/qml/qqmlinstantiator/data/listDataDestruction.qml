import QtQml

QtObject {
    id: menu

    Component.onCompleted: dt = new Date();
    property date dt
    property Instantiator i: Instantiator {
        model: {
            var model = [];
            var d = menu.dt;
            model.push({text: "A"});
            return model;
        }
        delegate: QtObject {
            objectName: modelData.text
            Component.onCompleted: menu.objectName = objectName
        }
    }
}
