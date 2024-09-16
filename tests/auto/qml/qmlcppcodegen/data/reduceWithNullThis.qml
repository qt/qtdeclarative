import QtQml

QtObject {
    id: mainItem
    property int topPadding: 12
    property int bottomPadding: 12

    property int preferredHeight: mainItem.children.reduce(maximumImplicitHeightReducer, 0) + topPadding + bottomPadding
    function maximumImplicitHeightReducer(accumulator: real, item: Binding): real {
        return Math.max(accumulator, (item.objectName + "b").length);
    }

    property int preferredHeight2: mainItem.children.reduce((accumulator, item) => {
        return Math.max(accumulator, (item.objectName + "b").length);
    }, 0) + topPadding + bottomPadding

    property list<Binding> children: [ Binding { objectName: "aaa" } ]
}
