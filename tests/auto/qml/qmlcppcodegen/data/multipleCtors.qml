pragma Strict

import TestTypes
import QtQml

QtObject {
    property rect r: Qt.rect(1, 2, 3, 4)
    property point p: Qt.point(5, 6);

    property withLength wr: r
    property withLength wp: p
    property withLength wi: 17
}
