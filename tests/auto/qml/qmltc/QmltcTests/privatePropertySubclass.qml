PrivateProperty {
    id: root
    property string dummy: "bar"
    strAlias: "foo" + dummy
    smthAlias: root.group.count - 12 + root.vt.count // should be 42
}
