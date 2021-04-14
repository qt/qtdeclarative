import QtQuick 2.0

DeprecatedFunctions {
    @Deprecated { reason: "No particular reason." }
    function deprecated(foobar) {}

    Component.onCompleted: {
        deprecated();
    }
}
