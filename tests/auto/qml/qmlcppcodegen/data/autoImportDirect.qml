import QtQml
import AutoImportTarget

QtObject {
    id: root
    property real num: 5
    property QtObject thing: Thing {
        value: root.num / 2
    }
}
