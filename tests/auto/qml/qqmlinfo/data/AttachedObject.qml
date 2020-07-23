import QtQuick 2.0
import org.qtproject.Test 1.0

Item {
    Attached.a: Attached.a

    Rectangle {
        width: height + 1
        height: width + 1
    }
}
