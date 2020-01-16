import QtQml.Models 2.12
import Qt.labs.qmlmodels 1.0
import QtQuick 2.12

Item {
    id: root
    width: 200
    height: 320

    property int numChanges: 0
    property bool ok: true

    DelegateModel {
        id: theModel

        model: ListModel {
            ListElement { role: "section" }
            ListElement { role: "item" }
            ListElement { role: "section" }
            ListElement { role: "item" }
            ListElement { role: "section" }
            ListElement { role: "item" }
            ListElement { role: "item" }
            ListElement { role: "item" }
        }

        filterOnGroup: "expanded"
        groups: DelegateModelGroup {
            name: "expanded"
        }

        delegate: DelegateChooser {
            role: "role"

            DelegateChoice {
                roleValue: "section"
                Text {
                    text: "+ Section " + index

                    Timer {
                        interval: (index + 10)
                        repeat: true
                        running: true
                        onTriggered: {
                            ++ root.numChanges;
                            if (model.role !== "section") {
                                root.ok = false;
                                console.warn("wrong!", root.numChanges);
                            }
                            let i = parent.DelegateModel.itemsIndex + 1;
                            for (; i < theModel.items.count; ++i) {
                                let item = theModel.items.get(i);
                                if (item.model.role === "section")
                                    break;
                                item.inExpanded = !item.inExpanded;
                            }
                        }
                    }
                }
            }

            DelegateChoice {
                roleValue: "item"
                Text {
                    text: "Item " + index
                }
            }
        }

        Component.onCompleted: items.addGroups(0, items.count, ["expanded"])
    }

    ListView {
        anchors.fill: parent
        model: theModel
    }
}
