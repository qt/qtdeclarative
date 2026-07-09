import QtQuick
import nested.module

Item {
    // Test is provided by nested.module, imported only from QML (no CMake
    // dependency). It resolves at runtime only if the nested.module import root
    // was written into qt.conf by the scanner.
    Test {}
}
