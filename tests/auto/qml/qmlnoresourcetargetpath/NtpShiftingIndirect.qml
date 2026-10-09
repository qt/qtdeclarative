pragma Strict
import QtQml
import NtpVersioned 2.0

QtObject {
    final property NtpShiftingHolder holder: NtpShiftingHolder {}
    property real shiftingLife: holder.shifting.life

    function poke(): void { holder.shifting.life = 1 }
}
