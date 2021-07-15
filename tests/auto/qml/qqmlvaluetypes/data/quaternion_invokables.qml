import QtQuick 2.0

Item {
    property bool success: false

    // parameters are copied from vector4d_invokables.qml, which are pretty much
    // useless as rotation quaternion, but should be good for testing invokables.
    property variant q1: Qt.quaternion(1, 2, 3, 4)
    property variant q2: Qt.quaternion(5, 6, 7, 8)
    property variant v1: Qt.vector3d(1, 2, 3)
    property real factor: 2.23

    Component.onCompleted: {
        let results = [];
        results.push(q1.dotProduct(q2) === 70);
        results.push(q1.times(q2) === Qt.quaternion(-60, 12, 30, 24));
        results.push(q1.times(v1) === Qt.vector3d(54, 60, 78));
        results.push(q1.times(factor) === Qt.quaternion(2.23, 4.46, 6.69, 8.92));
        results.push(q1.plus(q2) === Qt.quaternion(6, 8, 10, 12));
        results.push(q1.minus(q2) === Qt.quaternion(-4, -4, -4, -4));
        results.push(q1.normalized().fuzzyEquals(Qt.quaternion(0.182574, 0.365148, 0.547723, 0.730297), 0.00001));
        results.push(q1.inverted().fuzzyEquals(Qt.quaternion(0.033333, -0.066666, -0.100000, -0.133333), 0.00001));
        results.push(q1.conjugated() === Qt.quaternion(1, -2, -3, -4));
        results.push(q1.length() !== q2.length());
        results.push(q1.length() === Qt.quaternion(4, 3, 2, 1).length());
        results.push(q1.toEulerAngles().fuzzyEquals(Qt.vector3d(-41.810314, 79.695144, 116.565048), 0.00001));
        results.push(q1.toVector4d() === Qt.vector4d(2, 3, 4, 1));
        results.push(!q1.fuzzyEquals(q2));
        results.push(q1.fuzzyEquals(q2, 4));
        success = results.every(p => p);
    }
}
