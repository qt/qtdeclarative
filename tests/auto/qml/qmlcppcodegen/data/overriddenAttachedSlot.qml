import QtQml
import TestTypes

QtObject {
    property int result: OverriddenSlotHolder.processData({ count: 3, enabled: true, value: 42 })
}
