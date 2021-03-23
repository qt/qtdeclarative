pragma pippo
import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    height: 480
    title: qsTr("Scroll")
    visible: true
    width: 640

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
                        return 45;
                    }

                    text: root.text

                    function f(v = 4) {
                        let c = 0;
                        return {
                            "a": function () {
                                if (b == 0)
                                    c += 78 * 5 * v;
                            }()
                        };
                    }
                }
            }
        }
    }
}
