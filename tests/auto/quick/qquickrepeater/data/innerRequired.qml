import QtQuick

Item {
    ListModel {
        id: myModel
        ListElement { type: "Dog"; age: 8; noise: "meow" }
        ListElement { type: "Cat"; age: 5; noise: "woof" }
    }

    component SomeDelegate: Item {
        required property int age
        property string text
    }

    component AnotherDelegate: Item {
        property int age
        property string text

        SomeDelegate {
            age: 0
            text: ""
        }
    }

    Repeater {
        id: repeater
        model: myModel
        delegate: AnotherDelegate {
            age: model.age
            text: model.noise
        }
    }
}
