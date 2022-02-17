pragma Strict
import QtQml

QtObject {
    property real implicitHeight: {
        return /*+ (control.implicitHeaderHeight > 0
              ? control.implicitHeaderHeight + control.spacing
              : 0)*/ 8;
    }
}
