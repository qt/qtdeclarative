import QtQml
import A

QtObject {
    // Type Child imported from A takes precedence over type Child indirectly imported from B.
    property QtObject childA: Child {}
    property string a: childA.objectName

    // ChildB only exists in B.
    property QtObject childB: ChildB {}
    property string b: childB.objectName
}
