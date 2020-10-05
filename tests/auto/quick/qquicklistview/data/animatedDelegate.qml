import QtQuick 2.12

Item {
    width: 640
    height: 480

    property var moreModel: [myDataModelContainer.createObject(null)]
    property Component myDataModelContainer: Component {
        QtObject {}
    }

    function refreshModel() {
        //copy contents of moreModel
        var list = moreModel.slice()

        moreModel = [myDataModelContainer.createObject(null), myDataModelContainer.createObject(null)]

        for (var i = 0; i < list.length; i++) {
            //console.log("trying to destroy ="+list[i])
            list[i].destroy()
        }
    }

    ListView {
        id: listView
        objectName: "listView"
        anchors.fill: parent
        model: moreModel
        focus: true

        delegate: AnimatedButton {
            color: ListView.isCurrentItem ? "red" : "black"
        }
    }
}
