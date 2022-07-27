import QtQuick
import QmltcTests

AliasBase {
    id: derived
    property alias alias2: derived.font.letterSpacing

    alias1: 4
    alias2: 4
}
