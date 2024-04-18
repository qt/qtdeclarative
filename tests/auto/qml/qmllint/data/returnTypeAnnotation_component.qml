import QtQml
import QtQuick

QtObject {
    id: root
    component Comp : Item { }
    property Comp c: Comp{ }
    function comp() { return root.c }
}
