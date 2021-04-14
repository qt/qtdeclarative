import QtQuick 2.0

DeprecatedFunctions {
    function deprecatedOverride(x, y, z) {}
    Component.onCompleted: {
        deprecatedOverride();
    }
}
