import QtQuick 2.15

Item {
    property int count: myModel.count
    component MyModel : ListModel {
        ListElement { a: 10 }
        ListElement { a: 12 }
    }

    MyModel {
        id: myModel
    }
}
