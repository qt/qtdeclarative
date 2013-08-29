!wince*:!ios:!if(win*:isEqual(QT_ARCH, "x86_64")): DEFINES += V4_ENABLE_JIT ENABLE_YARR_JIT=1
else: DEFINES += ENABLE_YARR_JIT=0

# On Qt/Android/ARM release builds are thumb and debug builds arm,
# but we'll force the JIT to always generate thumb2
android:isEqual(QT_ARCH, "arm") {
    DEFINES += WTF_CPU_ARM_THUMB2
}

DEFINES += WTF_EXPORT_PRIVATE="" JS_EXPORT_PRIVATE=""

win*: DEFINES += NOMINMAX

DEFINES += ENABLE_LLINT=0
DEFINES += ENABLE_DFG_JIT=0
DEFINES += ENABLE_JIT_CONSTANT_BLINDING=0
DEFINES += ENABLE_ASSEMBLER=1
DEFINES += BUILDING_QT__

DEFINES += ENABLE_JIT=1

INCLUDEPATH += $$PWD/jit
INCLUDEPATH += $$PWD/assembler
INCLUDEPATH += $$PWD/runtime
INCLUDEPATH += $$PWD/wtf
INCLUDEPATH += $$PWD/stubs
INCLUDEPATH += $$PWD/stubs/wtf
INCLUDEPATH += $$PWD

disassembler:if(isEqual(QT_ARCH, "i386")|isEqual(QT_ARCH, "x86_64")):!win*: DEFINES += WTF_USE_UDIS86=1
else: DEFINES += WTF_USE_UDIS86=0

INCLUDEPATH += $$PWD/disassembler
INCLUDEPATH += $$PWD/disassembler/udis86
INCLUDEPATH += $$_OUT_PWD

win32-msvc2008|wince*: INCLUDEPATH += $$PWD/stubs/compat
