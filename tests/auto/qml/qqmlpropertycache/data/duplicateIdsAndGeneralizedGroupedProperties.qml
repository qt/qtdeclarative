import QtQuick 2.15

Item {
    component First : Item {
        Item {
            id: a
        }

        states: [
            State {
                name: "test1"

                PropertyChanges {
                    a.enabled: false
                }
            }
        ]
    }

    component Second : Item {
        QtObject {
            id: a
            property bool enabled: true
        }

        states: [
            State {
                name: "test2"

                PropertyChanges {
                    a.enabled: false
                }
            }
        ]

        property Component cc: Item {
            Item { id: a }

            states: [
                State {
                    name: "test3"

                    PropertyChanges {
                        a.enabled: false
                    }
                }
            ]
        }
    }

    First { id: first }
    Second { id: second }
    property Item third: second.cc.createObject();

    Component.onCompleted: {
        console.log(1, first.data[0].enabled, second.data[0].enabled, third.data[0].enabled);
        first.state = "test1";
        console.log(2, first.data[0].enabled, second.data[0].enabled, third.data[0].enabled);
        second.state = "test2";
        console.log(3, first.data[0].enabled, second.data[0].enabled, third.data[0].enabled);
        third.state = "test3";
        console.log(4, first.data[0].enabled, second.data[0].enabled, third.data[0].enabled);
    }
}
