import QtQml

QtObject {
    property string property_not_shadowed
    function method_not_shadowed(foo) {}

    property string property_shadowed
    function method_shadowed(foo) {}
}
