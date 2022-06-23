import QtQml
import QmltcTests

TypeWithMemberProperties {
    id: root
    x: 42
    y: "fourty two"
    property alias xAlias: root.x
    property alias yAlias: root.y
}
