import QtQuick 2.0

Item {
    @Deprecated { reason: "This deprecation should be overridden!" }
    function deprecatedOverride(a, b) {}

    @Deprecated { reason: "This deprecation should be visible!" }
    function deprecatedInherited(c, d) {}
}
