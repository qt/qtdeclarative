

#### Inputs



#### Libraries



#### Tests

# cxx14_make_unique
qt_config_compile_test(cxx14_make_unique
    LABEL "C++14 make_unique()"
"
#include <memory>

int main(int argc, char **argv)
{
    (void)argc; (void)argv;
    /* BEGIN TEST: */
std::unique_ptr<int> ptr = std::make_unique<int>();
    /* END TEST: */
    return 0;
}
")

# pointer_32bit
qt_config_compile_test(pointer_32bit
    LABEL "32bit pointers"
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

qt_feature("cxx14_make_unique" PRIVATE
    LABEL "C++14 make_unique"
    CONDITION QT_FEATURE_cxx14 OR TEST_cxx14_make_unique
)
qt_feature("qml_network" PUBLIC
    SECTION "QML"
    LABEL "QML network support"
    PURPOSE "Provides network transparency."
    CONDITION QT_FEATURE_network
)
# On arm and arm64 we need a specialization of cacheFlush() for each OS to be enabeled. Therefore the config white list. Also Mind that e.g. x86_32 has arch.x86_64 but 32bit pointers. Therefore the checks for architecture and pointer size. Finally, ios and tvos can technically use the JIT but Apple does not allow it. Therefore, it's disabled by default.
qt_feature("qml_jit" PRIVATE
    SECTION "QML"
    LABEL "QML just-in-time compiler"
    PURPOSE "Provides a JIT for QML and JavaScript"
    AUTODETECT NOT APPLE_IOS AND NOT APPLE_TVOS
    CONDITION ( ( ( TEST_architecture_arch STREQUAL i386 ) AND TEST_pointer_32bit AND QT_FEATURE_sse2 ) OR ( ( TEST_architecture_arch STREQUAL x86_64 ) AND TEST_pointer_64bit AND QT_FEATURE_sse2 ) OR ( ( TEST_architecture_arch STREQUAL arm ) AND TEST_pointer_32bit AND TEST_arm_fp AND TEST_arm_thumb AND ( LINUX OR APPLE_IOS OR APPLE_TVOS OR QNX ) ) OR ( ( TEST_architecture_arch STREQUAL arm64 ) AND TEST_pointer_64bit AND TEST_arm_fp AND ( LINUX OR APPLE_IOS OR APPLE_TVOS OR QNX OR INTEGRITY ) ) )
)
qt_feature("qml_debug" PUBLIC
    SECTION "QML"
    LABEL "QML debugging and profiling support"
    PURPOSE "Provides infrastructure and plugins for debugging and profiling."
)
qt_feature("qml_profiler" PRIVATE
    SECTION "QML"
    LABEL "Command line QML Profiler"
    PURPOSE "Supports retrieving QML tracing data from an application."
    CONDITION ( QT_FEATURE_commandlineparser ) AND ( QT_FEATURE_qml_debug ) AND ( QT_FEATURE_qml_network AND QT_FEATURE_localserver ) AND ( QT_FEATURE_xmlstreamwriter )
)
qt_feature("qml_preview" PRIVATE
    SECTION "QML"
    LABEL "Command line QML Preview tool"
    PURPOSE "Updates QML documents in your application live as you change them on disk"
    CONDITION ( QT_FEATURE_commandlineparser ) AND ( QT_FEATURE_filesystemwatcher ) AND ( QT_FEATURE_qml_network AND QT_FEATURE_localserver ) AND ( QT_FEATURE_process ) AND ( QT_FEATURE_qml_debug )
)
qt_feature("qml_devtools" PRIVATE
    SECTION "QML"
    LABEL "QML Development Tools"
    PURPOSE "Provides the QmlDevtools library and various utilities."
)
qt_feature("qml_sequence_object" PRIVATE
    SECTION "QML"
    LABEL "QML sequence object"
    PURPOSE "Supports mapping sequence types into QML."
)
qt_feature("qml_xml_http_request" PRIVATE
    SECTION "QML"
    LABEL "QML XML http request"
    PURPOSE "Provides support for sending XML http requests."
    CONDITION ( QT_FEATURE_xmlstreamreader ) AND ( QT_FEATURE_qml_network )
)
qt_feature("qml_locale" PRIVATE
    SECTION "QML"
    LABEL "QML Locale"
    PURPOSE "Provides support for locales in QML."
)
qt_feature("qml_animation" PRIVATE
    SECTION "QML"
    LABEL "QML Animations"
    PURPOSE "Provides support for animations and timers in QML."
    CONDITION QT_FEATURE_animation
)
qt_feature("qml_worker_script" PRIVATE
    SECTION "QML"
    LABEL "QML WorkerScript"
    PURPOSE "Enables the use of threads in QML."
    CONDITION QT_FEATURE_thread
)
qt_feature("qml_itemmodel" PRIVATE
    SECTION "QML"
    LABEL "QML Item Model"
    PURPOSE "Provides the item model for item views in QML"
    CONDITION QT_FEATURE_itemmodel
)
