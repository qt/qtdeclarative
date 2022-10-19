import QtQuick 2.15

Item {
    width: 800
    height: 600

    Component {
        id: dyn
        Item {
            property Component comp: Item {}
        }
    }

    TableView {
        id: tv
        anchors.fill: parent
	objectName: "tableview"
    }

    Component.onCompleted: {
        let o = dyn.createObject();
        tv.delegate = o.comp;
        o.destroy();
    }
}
