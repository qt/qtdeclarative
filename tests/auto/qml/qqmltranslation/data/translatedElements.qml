import QtQml
import QtQml.Models

DelegateModel {
    model: ListModel {
        ListElement { dish: qsTr("soup"); price: 60 }
        ListElement { dish: qsTr("fish"); price: 100 }
        ListElement { dish: qsTr("meat"); price: 230 }
        ListElement { dish: qsTr("bread"); price: 10 }
        ListElement { dish: qsTranslate("ctx", "sizzling hot pebbles"); price: 12 }
    }

    delegate: QtObject {
        required property string dish
    }
}
