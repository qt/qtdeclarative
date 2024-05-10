import QtQml

QtObject {
    id: mainItem

    function arg(item: Binding) : QtObject { return item }
    function ret(item: QtObject) : Binding { return item }

    property QtObject a: arg(mainItem);
    property QtObject b: ret(mainItem);
}
