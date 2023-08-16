pragma pippo
import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    visible: true
    width: 640
    height: 480
    title: qsTr("Scroll")

    property var arr: [1, 2, 3]
    property var arrTrailingComma: [1, 2, 3,]

    Rectangle {
        anchors.fill: parent

        Behavior on opacity {
        }

        ListView {
            width: parent.width
            model: {
                MySingleton.mySignal();
                20;
            }
            delegate: ItemDelegate {
                id: root
                text: "Item " + (index + 1)
                width: parent.width
                Rectangle {
                    text: "bla"
                }
                MyComponent {
                    text: root.text
                    function f(v = 4) {
                        let c = 0;
                        return {
                            a: function () {
                                if (b == 0)
                                    c += 78 * 5 * v;
                            }()
                        };
                    }
                    property int a: {
                        return 45;
                    }
                }
            }
        }
    }
}
