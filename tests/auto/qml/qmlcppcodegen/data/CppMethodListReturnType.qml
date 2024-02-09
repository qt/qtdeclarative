pragma Strict

import QtQuick
import TestTypes

Item {
    ListProvider {
        id: listProvider
    }

    property var list: listProvider.intList()
}
