import QtQml
import Qt.example.qobjectSingleton 1.0

QtObject {
    property int someValue: MyApi.someProperty
    property int doneSomething: MyApi.doSomething()
}
