set(cpp_sources
    birthdayparty.cpp birthdayparty.h
    cppbaseclass.h
    objectwithmethod.h
    person.cpp person.h
    theme.cpp theme.h
    timelinetheme.cpp timelinetheme.h
)

set(qml_files
    AccessModelMethodsFromOutside.qml
    BaseMember.qml
    BindingExpression.qml
    Dummy.qml
    Enums.qml
    Loopy.qml
    Panel.qml
    ProgressBar/Keyframe.qml
    ProgressBar/KeyframeGroup.qml
    ProgressBar/ProgressBar.ui.qml
    ProgressBar/Root.qml
    ProgressBar/Timeline.qml
    ProgressBar/TimelineAnimation.qml
    RootWithoutId.qml
    SelectionRectangle.qml
    Test.qml
    TestCase.qml
    aliasLookup.qml
    anchorsFill.qml
    array.qml
    asCast.qml
    attachedBaseEnum.qml
    bindToValueType.qml
    callContextPropertyLookupResult.qml
    childobject.qml
    colorAsVariant.qml
    colorString.qml
    componentReturnType.qml
    compositeTypeMethod.qml
    compositesingleton.qml
    construct.qml
    contextParam.qml
    conversions.qml
    conversions2.qml
    curlygrouped.qml
    deadShoeSize.qml
    dialog.qml
    dynamicscene.qml
    enumInvalid.qml
    enumScope.qml
    enumsInOtherObject.qml
    enumsUser.qml
    equalsUndefined.qml
    excessiveParameters.qml
    extendedTypes.qml
    failures.qml
    fileDialog.qml
    funcWithParams.qml
    functionReturningVoid.qml
    globals.qml
    idAccess.qml
    immediateQuit.qml
    imports/QmlBench/Globals.qml
    importsFromImportPath.qml
    intEnumCompare.qml
    intOverflow.qml
    interactive.qml
    interceptor.qml
    jsMathObject.qml
    jsimport.qml
    jsmoduleimport.qml
    layouts.qml
    library.js
    listIndices.qml
    listlength.qml
    math.qml
    methods.qml
    modulePrefix.qml
    noBindingLoop.qml
    noQQmlData.qml
    nonNotifyable.qml
    noscope.qml
    nullAccess.qml
    outOfBounds.qml
    overriddenMember.qml
    ownProperty.qml
    page.qml
    parentProp.qml
    pressAndHoldButton.qml
    registerelimination.qml
    scopeVsObject.qml
    script.js
    script.mjs
    shared/Slider.qml
    shifts.qml
    signal.qml
    signalHandler.qml
    specificParent.qml
    stringLength.qml
    testlogger.js
    text.qml
    themerbad.qml
    themergood.qml
    undefinedResets.qml
    unknownParameter.qml
    unusedAttached.qml
    urlString.qml
    valueTypeProperty.qml
    voidfunction.qml
)

set(resource_files
    ProgressBar/built-with-Qt_Large.png
    imports/QmlBench/qmldir
)

set_source_files_properties("shared/Slider.qml"
    PROPERTIES QT_RESOURCE_ALIAS "Slider.qml"
)

qt_add_library(codegen_test_module STATIC)
qt_autogen_tools_initial_setup(codegen_test_module)

set_target_properties(codegen_test_module PROPERTIES
    # We really want qmlcachegen here, even if qmlsc is available
    QT_QMLCACHEGEN_EXECUTABLE qmlcachegen
)

qt6_add_qml_module(codegen_test_module
    VERSION 1.0
    URI TestTypes
    IMPORT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/imports/"
    SOURCES
        ${cpp_sources}
    QML_FILES
        ${qml_files}
    RESOURCES
        ${resource_files}
    OUTPUT_DIRECTORY TestTypes # Make sure tst_qmlcachegen doesn't see our output
)

qt_autogen_tools_initial_setup(codegen_test_moduleplugin)
