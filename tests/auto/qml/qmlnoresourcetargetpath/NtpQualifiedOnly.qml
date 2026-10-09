pragma Strict
import QtQml
import NtpVersioned as V

QtObject {
    property V.Late late: V.Late {}
    property real lateLife: late.life

    function poke(): void { late.life = 1 }
}
