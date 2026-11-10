import QtQml
import Test

QtObject {
    readonly property QtObject named: QtObject { id: foo }
    readonly property TestObject load: TestObject { scriptString: foo }
}
