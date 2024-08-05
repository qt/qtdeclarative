import QtQml
import OnlyDeclarative

QtObject {
    property QtObject o: ObjectListArgumentMethod {
        id: list
    }

    Component.onCompleted: console.log(list.method(list.objects));
}
