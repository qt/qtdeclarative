import QmltcTests 1.0
PrivatePropertyType {
    id: root
    vt.count: 11
    group.count: 43
    foo: "Smth is: " + smth

    property alias strAlias: root.group.str
    property alias smthAlias: root.smth
}
