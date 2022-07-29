import QtQml
import QmltcTests

TypeWithSpecialProperties {
    id: root
    x: 42
    y: "fourty two"
    z: 3.2
    property alias xAlias: root.x
    property alias yAlias: root.y
    property alias zAlias: root.z

    property alias xxAlias: root.xx
    property alias xyAlias: root.xy
    xyAlias: undefined
}
