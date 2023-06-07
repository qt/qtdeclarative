import QtQml
import Test

UnregisteredValueTypeHandler {
    Component.onCompleted: consume(produce())
}
