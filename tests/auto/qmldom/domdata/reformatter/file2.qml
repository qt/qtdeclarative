import QtQuick 2.15
import QtQuick.Window 2.15

Window {
    visible: true
    width: 640
    height: 480
    title: qsTr("Scroll")

    onTrigger: console.log("trigger signal emitted")

    onSend: {
            console.log("send signal emitted with notice: " + notice)
    }

    Rectangle {
        anchors.fill: parent

        ListView {
            width: parent.width
            model: {
                MySingleton.mySignal()
                20
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
                    function f(v){
                        let c = 0
                        return {
                            a: function(){ if (b == 0) c += 78*5*v; }()
                        }
                    }
                    property int a: {
                        let x = isNaN;
                        (45)
                        x ? 5 + 1 : 8
                    }
                    property list<Item> b: [ Item{
                        width: 5
                    },
                    Item{
                        width: 6
                    }]
                }
            }
        }
    }
    Component.onCompleted: {
        console.log("loaded")
    }
}
