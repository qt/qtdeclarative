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

    ListView {
        id: listView
        model: myModel
        width: 100
        height: 100
        delegate: AnotherDelegate {
            age: model.age
            text: model.noise
        }
    }
}
