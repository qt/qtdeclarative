import QtQml
import qqmllanguage.Test

UnregisteredValueTypeHandler {
    Component.onCompleted: {
        consume(produce())
        consume(produceDerived())
        consume(produceGadgeted())
    }
}
