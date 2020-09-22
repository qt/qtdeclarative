import Qt.test 1.0
ClassWithQProperty {
    id: root
    property alias myAlias: root.value
    property int changeCounter: 0
    onMyAliasChanged: ++changeCounter
}
