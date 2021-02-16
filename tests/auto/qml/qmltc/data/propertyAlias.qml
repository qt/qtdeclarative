import QtQml 2.0

QtObject {
    id: root
    property int dummy: 12
    property int origin: dummy / 2
    property alias aliasToOrigin: root.origin // NB: qualified access is a must

    function resetAliasToConstant() {
        aliasToOrigin = 42;
    }

    function resetOriginToConstant() {
        origin = 189;
    }

    function resetAliasToNewBinding() {
        aliasToOrigin = Qt.binding(function() { return dummy * 3 });
    }

    function resetOriginToNewBinding() {
        origin = Qt.binding(function() { return dummy });
    }

    function getAliasValue() {
        return aliasToOrigin;
    }
}
