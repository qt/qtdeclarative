import QtQuick

Item  {
    Loader {
        active: loaderActive
        sourceComponent: comp
    }

    Component {
        id: comp
        Column {
            Repeater {
                id: repeater
                model: cppModel

                Component.onCompleted: console.log("Repeater constructed");
                Component.onDestruction: console.log("Repeater destroyed");

                delegate: Text {
                    text: {
                        console.log("updating text");
                        return display + rootData.getValue();
                    }
                }
            }
        }
    }
}
