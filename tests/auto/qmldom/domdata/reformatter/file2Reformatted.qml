import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    height: 480
    title: qsTr("Scroll")
    visible: true
    width: 640

    Component.onCompleted: {
        console.log("loaded");
    }
    onSend: {
        console.log("send signal emitted with notice: " + notice);
    }
    onTrigger: console.log("trigger signal emitted")

    Rectangle {
        anchors.fill: parent

        ListView {
            model: {
                MySingleton.mySignal();
                20;
            }
            width: parent.width

            delegate: ItemDelegate {
                id: root

                text: "Item " + (index + 1)
                width: parent.width

                Rectangle {
                    text: "bla"
                }
                MyComponent {
                    property int a: {
                        let x = isNaN;
                        (45);
                        x ? 5 + 1 : 8;
                    }
                    property list<Item> b: [
                        Item {
                            width: 5
                        },
                        Item {
                            width: 6
                        }
                    ]

                    function f(v) {
                        let c = 0;
                        return {
                            a: function () {
                                if (b == 0)
                                    c += 78 * 5 * v;
                            }()
                        };
                    }

                    text: root.text
                }
            }
        }
    }
}
