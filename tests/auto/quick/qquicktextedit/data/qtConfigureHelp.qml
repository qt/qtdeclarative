import QtQuick
import Qt.test 1.0

NodeCheckerTextEdit {
    id: textedit
    width: 640
    height: 480
    text: "Usage:  configure [options] [-- cmake-options]

This is a convenience script for configuring Qt with CMake.
Options after the double dash are directly passed to CMake.

Top-level installation directories:
    -prefix <dir> ...... The deployment directory, as seen on the target device.
                        [/usr/local/Qt-$QT_VERSION; qtbase build directory if
                        -developer-build]
    -no-prefix ......... The deployment directory is set to the qtbase build
                        directory. Can be used instead of -developer-build
                        to not have to install, as well as avoid
                        -developer-build's default of -warnings-are-errors.
    -extprefix <dir> ... The installation directory, as seen on the host machine.
                        [SYSROOT/PREFIX]

Fine tuning of installation directory layout. Note that all directories
except -sysconfdir should be located under -prefix:

    -bindir <dir> ......... Executables [PREFIX/bin]
    -headerdir <dir> ...... Header files [PREFIX/include]
    -libdir <dir> ......... Libraries [PREFIX/lib]
    -archdatadir <dir> .... Arch-dependent data [PREFIX]
    -plugindir <dir> ...... Plugins [ARCHDATADIR/plugins]
    -libexecdir <dir> ..... Helper programs [ARCHDATADIR/bin on Windows,
                            ARCHDATADIR/libexec otherwise]
    -qmldir <dir> ......... QML imports [ARCHDATADIR/qml]
    -datadir <dir> ........ Arch-independent data [PREFIX]
    -docdir <dir> ......... Documentation [DATADIR/doc]
    -translationdir <dir> . Translations [DATADIR/translations]
    -sysconfdir <dir> ..... Settings used by Qt programs [PREFIX/etc/xdg]
    -examplesdir <dir> .... Examples [PREFIX/examples]
    -testsdir <dir> ....... Tests [PREFIX/tests]
    -hostdatadir <dir> .... Data used by qmake [PREFIX]

Conventions for the remaining options: When an option's description is
followed by a list of values in brackets, the interpretation is as follows:
'yes' represents the bare option; all other values are possible prefixes to
the option, e.g., -no-gui. Alternatively, the value can be assigned, e.g.,
--gui=yes. Values are listed in the order they are tried if not specified;
'auto' is a shorthand for 'yes/no'. Solitary 'yes' and 'no' represent binary
options without auto-detection.

Configure meta:

    -help, -h ............ Display this help screen
    -redo ................ Re-configure with previously used options. In addition,
                            redo removes CMakeCache.txt file and CMakeFiles/ directory
                            and recreates them from scratch.
                            Additional options may be passed, but will not be
                            saved for later use by -redo.

    -feature-<feature> ... Enable <feature>
    -no-feature-<feature>  Disable <feature> [none]
    -list-features ....... List available features. Note that some features
                            have dedicated command line options as well.

Build options:

    -cmake-generator <name> ... Explicitly specify the build system generator for
                            CMake instead of auto-detecting one.
    -cmake-use-default-generator ... Turn off auto-detection of the CMake build
                            system generatsr.
    -cmake-file-api ...... Let CMake store build metadata for loading the build
                            into an IDE. [no; yes if -developer-build]
    -no-guess-compiler ... Do not guess the compiler from the target mkspec.
    -release ............. Build Qt with debugging turned off [yes]
    -debug ............... Build Qt with debugging turned on [no]
    -debug-and-release ... Build two versions of Qt, with and without
                            debugging turned on [yes] (Apple and Windows only)
    -optimize-debug ...... Enable debug-friendly optimizations in debug builds
                            [auto] (Not supported with MSVC or Clang toolchains)
    -optimize-size ....... Optimize release builds for size instead of speed [no]
    -force-debug-info .... Create symbol files for release builds [no]
    -separate-debug-info . Split off debug information to separate files [no]
    -gdb-index ........... Index the debug info to speed up GDB
                            [no; auto if -developer-build with debug info]
    -gc-binaries ......... Place each function or data item into its own section
                            and enable linker garbage collection of unused
                            sections. [auto for static builds, otherwise no]
    -force-asserts ....... Enable Q_ASSERT even in release builds [no]
    -developer-build ..... Compile and link Qt for developing Qt itself
                            (exports for auto-tests, extra checks, etc.) [no]

    -shared .............. Build shared Qt libraries [yes] (no for UIKit)
    -static .............. Build static Qt libraries [no] (yes for UIKit)
    -framework ........... Build Qt framework bundles [yes] (Apple only)

    -platform <target> ... Select mkspec for the qmake companion files
    -device <name> ....... Select devices/mkspec for the qmake companion files
    -device-option <key=value> ... Add option for the device mkspec

    -appstore-compliant .. Disable code that is not allowed in platform app stores.
                            This is on by default for platforms which require distribution
                            through an app store by default, in particular Android,
                            iOS, tvOS, and watchOS. [auto]

    -qt-host-path <path> . Specify path to a Qt host build for cross-compiling.
    -qtnamespace <name> .. Wrap all Qt library code in 'namespace <name> {...}'.
    -qtlibinfix <infix> .. Rename all libQt6*.so to libQt6*<infix>.so.

    -testcocoon .......... Instrument with the TestCocoon code coverage tool [no]
    -gcov ................ Instrument with the GCov code coverage tool [no]

    -trace [backend] ..... Enable instrumentation with tracepoints.
                            Currently supported backends are 'etw' (Windows) and
                            'lttng' (Linux), or 'yes' for auto-detection. [no]

    -sanitize {address|thread|memory|fuzzer-no-link|undefined}
                            Instrument with the specified compiler sanitizer.
                            Note that some sanitizers cannot be combined;
                            for example, -sanitize address cannot be combined with
                            -sanitize thread.

    -mips_dsp/-mips_dspr2  Use MIPS DSP/rev2 instructions [auto]

    -qreal <type> ........ typedef qreal to the specified type. [double]
                            Note: this affects binary compatibility.

    -R <string> .......... Add an explicit runtime library path to the Qt
                            libraries. Supports paths relative to LIBDIR.
    -rpath ............... Link Qt libraries and executables using the library
                            install path as a runtime library path. Similar to
                            -R LIBDIR. On Apple platforms, disabling this implies
                            using absolute install names (based in LIBDIR) for
                            dynamic libraries and frameworks. [auto]

    -reduce-exports ...... Reduce amount of exported symbols [auto]
    -reduce-relocations .. Reduce amount of relocations [auto] (Unix only)

    -plugin-manifests .... Embed manifests into plugins [no] (Windows only)
    -static-runtime ...... With -static, use static runtime [no] (Windows only)

    -pch ................. Use precompiled headers [auto]
    -ltcg ................ Use Link Time Code Generation [no]
    -intelcet ............ Use Intel Control-flow Enforcement Technology [no]
    -linker [bfd,gold,lld,mold]
                            Force use of the GNU ld, GNU gold, LLVM/LLD or mold
                            linker instead of default one (GCC and clang only)
    -ccache .............. Use the ccache compiler cache [no] (Unix only)
    -unity-build ......... Enable Unity (Jumbo) build
    -unity-build-batch-size <int>
                            Maximum number of source files used by the unity build
                            to create unity source files [8]

    -warnings-are-errors . Treat warnings as errors [no; yes if -developer-build]

    -disable-deprecated-up-to <version>
                            Set the QT_DISABLE_DEPRECATED_UP_TO value to <version>.
                            QT_DISABLE_DEPRECATED_UP_TO is used to remove
                            deprecated methods from both API and ABI.
                            <version> is a hex value, for example 0x060500 can be
                            used to remove all code deprecated in Qt 6.5.0 or
                            earlier releases.
                            By default <version> is set to 0x040000 and 0x050000 on
                            Windows, and non-Windows respectively.
    -disable-deprecated-up-to <version>
                            Set the QT_DISABLE_DEPRECATED_UP_TO value to <version>.
                            QT_DISABLE_DEPRECATED_UP_TO is used to remove
                            deprecated methods from both API and ABI.
                            <version> is a hex value, for example 0x060500 can be
                            used to remove all code deprecated in Qt 6.5.0 or
                            earlier releases.
                            By default <version> is set to 0x040000 and 0x050000 on
                            Windows, and non-Windows respectively.
used to remove all code deprecated in Qt 6.5.0 or
                            earlier releases.

Build environment:

    -sysroot <dir> ....... Set <dir> as the target sysroot

    -pkg-config .......... Use pkg-config [auto] (Unix only)

    -D <string> .......... Pass additional preprocessor define
    -I <string> .......... Pass additional include path
    -L <string> .......... Pass additional library path
    -F <string> .......... Pass additional framework path (Apple only)>"
}
