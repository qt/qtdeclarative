pragma Strict
import QtQml

QtObject {
    property alias burnerLife: burner.life

    // Property lookup on a composite type of the same module
    function poke(): void { burner.life = 1 }

    property QtObject child: NtpBurner { id: burner }
}
