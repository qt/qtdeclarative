# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause



#### Inputs



#### Libraries


qt_find_package(LTTngUST PROVIDED_TARGETS LTTng::UST MODULE_NAME qml QMAKE_LIB lttng-ust)
qt_find_package(Python REQUIRED)
if(Python_Interpreter_FOUND)
    # Need to make it globally available to the project
    set(QT_INTERNAL_DECLARATIVE_PYTHON "${Python_EXECUTABLE}" CACHE STRING "")
endif()

#### Tests

# pointer_32bit
qt_config_compile_test(pointer_32bit
    LABEL "32bit pointers"
    CODE
"


int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    /* BEGIN TEST: */
static_assert(sizeof(void *) == 4, \"fail\");
    /* END TEST: */
    return 0;
}
")

# pointer_64bit
qt_config_compile_test(pointer_64bit
    LABEL "64bit pointers"
    CODE
"


int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    /* BEGIN TEST: */
static_assert(sizeof(void *) == 8, \"fail\");
    /* END TEST: */
    return 0;
}
")

# arm_thumb
qt_config_compile_test(arm_thumb
    LABEL "THUMB mode on ARM"
    CODE
"


int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    /* BEGIN TEST: */
#if defined(thumb2) || defined(__thumb2__)
#    define THUMB_OK
#elif (defined(__thumb) || defined(__thumb__)) && __TARGET_ARCH_THUMB-0 == 4
#    define THUMB_OK
#elif defined(__ARM_ARCH_ISA_THUMB) && __ARM_ARCH_ISA_THUMB == 2
// clang 3.5 and later will set this if the core supports the Thumb-2 ISA.
#    define THUMB_OK
#else
#    error \"fail\"
#endif
    /* END TEST: */
    return 0;
}
")

# arm_fp
qt_config_compile_test(arm_fp
    LABEL "Sufficiently recent FPU on ARM"
    CODE
"


int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    /* BEGIN TEST: */
// if !defined(__ARM_FP) we might be on MSVC or we might have a device
// without an FPU.
// TODO: The latter case is not supported, but the test still succeeds.
#if defined(__ARM_FP) && (__ARM_FP <= 0x04)
#    error \"fail\"
#endif
    /* END TEST: */
    return 0;
}
")



#### Features

qt_feature("qml-network" PUBLIC
    SECTION "QML"
    LABEL "QML network support"
    PURPOSE "Provides network transparency."
    CONDITION QT_FEATURE_network
)
# On arm and arm64 we need a specialization of cacheFlush() for each OS to be enabeled. Therefore the config white list. Also Mind that e.g. x86_32 has arch.x86_64 but 32bit pointers. Therefore the checks for architecture and pointer size. Finally, ios and tvos can technically use the JIT but Apple does not allow it. Therefore, it's disabled by default.
qt_feature("qml-jit" PRIVATE
    SECTION "QML"
    LABEL "QML just-in-time compiler"
    PURPOSE "Provides a JIT for QML and JavaScript"
    AUTODETECT NOT IOS AND NOT TVOS
    CONDITION ( ( ( TEST_architecture_arch STREQUAL i386 ) AND TEST_pointer_32bit AND QT_FEATURE_sse2 ) OR ( ( TEST_architecture_arch STREQUAL x86_64 ) AND TEST_pointer_64bit AND QT_FEATURE_sse2 ) OR ( ( TEST_architecture_arch STREQUAL arm ) AND TEST_pointer_32bit AND TEST_arm_fp AND TEST_arm_thumb AND ( ANDROID OR LINUX OR IOS OR TVOS OR QNX ) ) OR ( ( TEST_architecture_arch STREQUAL arm64 ) AND TEST_pointer_64bit AND TEST_arm_fp AND ( ANDROID OR LINUX OR IOS OR TVOS OR QNX OR INTEGRITY ) ) )
)
# special case begin
# When doing macOS universal builds, JIT needs to be disabled for the ARM slice.
# Because both arm and x86_64 slices are built in one clang frontend invocation
# we need this hack to ensure each backend invocation sees the correct value
# of the feature definition.
qt_extra_definition("QT_QML_JIT_SUPPORTED_IMPL" "0
// Unset dummy value
#undef QT_QML_JIT_SUPPORTED_IMPL
// Compute per-arch value and save in extra define
#if QT_CONFIG(qml_jit) && !(defined(Q_OS_MACOS) && defined(Q_PROCESSOR_ARM))
#define QT_QML_JIT_SUPPORTED_IMPL 1
#else
#define QT_QML_JIT_SUPPORTED_IMPL 0
#endif
// Unset original feature value
#undef QT_FEATURE_qml_jit
// Set new value based on previous computation
#if QT_QML_JIT_SUPPORTED_IMPL
#define QT_FEATURE_qml_jit 1
#else
#define QT_FEATURE_qml_jit -1
#endif
" PRIVATE)
# special case end
qt_feature("qml-debug" PUBLIC
    SECTION "QML"
    LABEL "QML debugging and profiling support"
    PURPOSE "Provides infrastructure and plugins for debugging and profiling."
)
qt_feature("qml-profiler" PRIVATE
    SECTION "QML"
    LABEL "Command line QML Profiler"
    PURPOSE "Supports retrieving QML tracing data from an application."
    CONDITION ( QT_FEATURE_commandlineparser ) AND ( QT_FEATURE_qml_debug ) AND ( QT_FEATURE_qml_network AND QT_FEATURE_localserver ) AND ( QT_FEATURE_xmlstreamwriter )
)
qt_feature("qml-preview" PRIVATE
    SECTION "QML"
    LABEL "Command line QML Preview tool"
    PURPOSE "Updates QML documents in your application live as you change them on disk"
    CONDITION ( QT_FEATURE_commandlineparser ) AND ( QT_FEATURE_filesystemwatcher ) AND ( QT_FEATURE_qml_network AND QT_FEATURE_localserver ) AND ( QT_FEATURE_process ) AND ( QT_FEATURE_qml_debug )
)
qt_feature("qml-xml-http-request" PRIVATE
    SECTION "QML"
    LABEL "QML XML http request"
    PURPOSE "Provides support for sending XML http requests."
    CONDITION ( QT_FEATURE_xmlstreamreader ) AND ( QT_FEATURE_qml_network )
)
qt_feature("qml-locale" PRIVATE
    SECTION "QML"
    LABEL "QML Locale"
    PURPOSE "Provides support for locales in QML."
)
qt_feature("qml-animation" PRIVATE
    SECTION "QML"
    LABEL "QML Animations"
    PURPOSE "Provides support for animations and timers in QML."
    CONDITION QT_FEATURE_animation
)
qt_feature("qml-worker-script" PRIVATE
    SECTION "QML"
    LABEL "QML WorkerScript"
    PURPOSE "Enables the use of threads in QML."
    CONDITION QT_FEATURE_thread
)
qt_feature("qml-itemmodel" PRIVATE
    SECTION "QML"
    LABEL "QML Item Model"
    PURPOSE "Provides the item model for item views in QML"
    CONDITION QT_FEATURE_itemmodel
)
qt_feature("qml-xmllistmodel" PRIVATE
    SECTION "QML"
    LABEL "QML XmlListModel"
    PURPOSE "Enable XmlListModel in QML"
    CONDITION QT_FEATURE_qml_itemmodel AND QT_FEATURE_future
)

qt_feature("qml-python" PRIVATE
    LABEL "python"
    CONDITION Python_Interpreter_FOUND
)
qt_configure_add_summary_section(NAME "Qt QML")
qt_configure_add_summary_entry(ARGS "qml-network")
qt_configure_add_summary_entry(ARGS "qml-debug")
qt_configure_add_summary_entry(ARGS "qml-jit")
qt_configure_add_summary_entry(ARGS "qml-xml-http-request")
qt_configure_add_summary_entry(ARGS "qml-locale")
qt_configure_end_summary_section() # end of "Qt QML" section
qt_configure_add_report_entry(
    TYPE ERROR
    MESSAGE "Python is required to build QtQml."
    CONDITION NOT QT_FEATURE_qml_python
)
