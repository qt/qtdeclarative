pragma Strict
import QtQml

QtObject {
    final property NtpLateHolder holder: NtpLateHolder {}
    property real lateLife: holder.late.life

    function poke(): void { holder.late.life = 1 }
}
