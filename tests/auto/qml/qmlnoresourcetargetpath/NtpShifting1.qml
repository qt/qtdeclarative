pragma Strict
import QtQml
import NtpVersioned 2.0 as Other
import NtpVersioned 1.0 as Used

QtObject {
    property Used.Shifting shifting: Used.Shifting {}
    property real shiftingLife: shifting.life

    function poke(): void { shifting.life = 1 }
}
