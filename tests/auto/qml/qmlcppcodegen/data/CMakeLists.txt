# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(cpp_sources
    ambiguous.h
    birthdayparty.cpp birthdayparty.h
    cppbaseclass.h
    druggeljug.h
    dummyobjekt.h
    dynamicmeta.h
    enumproblems.h
    enumProperty.h
    gadgetwithenum.h
    invisible.h
    multiforeign.h
    objectwithmethod.h
    person.cpp person.h
    sequenceToIterable.h
    sequencetypeexample.cpp sequencetypeexample.h
    state.h
    theme.cpp theme.h
    timelinetheme.cpp timelinetheme.h
    variantMapLookup.h
    variantreturn.h
    weathermoduleurl.h
    wrapwithvariant.h
    withlength.h
)

set(qml_files
    AccessModelMethodsFromOutside.qml
    ArraySequenceLengthInterop.qml
    BadType.qml
    BaseMember.qml
    BindingExpression.qml
    CxxTypeFromDir.qml
    CxxTypeFromImplicit.qml
    Cycle1.qml
    Cycle2.qml
    Cycle3.qml
    Dummy.qml
    Enums.qml
    Foozle.qml
    Loopy.qml
    OkType.qml
    Panel.qml
    Planner.qml
    ProgressBar/Keyframe.qml
    ProgressBar/KeyframeGroup.qml
    ProgressBar/ProgressBar.ui.qml
    ProgressBar/Root.qml
    ProgressBar/Timeline.qml
    ProgressBar/TimelineAnimation.qml
    RootWithoutId.qml
    SelectionRectangle.qml
    ShadowedObjectName.qml
    ShadowedObjectNameDerived.qml
    Test.qml
    TestCase.qml
    WindowDerived.qml
    aliasLookup.qml
    ambiguous1/Ambiguous.qml
    ambiguous2/Ambiguous.qml
    ambiguousAs.qml
    ambiguousSignals.qml
    anchorsFill.qml
    argumentConversion.qml
    array.qml
    asCast.qml
    attachedBaseEnum.qml
    badSequence.qml
    basicBlocksWithBackJump.qml
    basicDTZ.qml
    bindToValueType.qml
    blockComments.qml
    boolCoercions.qml
    boolPointerMerge.qml
    boundComponents.qml
    callContextPropertyLookupResult.qml
    callWithSpread.qml
    childobject.qml
    colorAsVariant.qml
    colorString.qml
    comparisonTypes.qml
    componentReturnType.qml
    compositeTypeMethod.qml
    compositesingleton.qml
    consoleObject.qml
    construct.qml
    contextParam.qml
    conversionDecrement.qml
    conversionInDeadCode.qml
    conversions.qml
    conversions2.qml
    convertToOriginalReadAcumulatorForUnaryOperators.qml
    curlygrouped.qml
    cycleHead.qml
    dateConversions.qml
    deadShoeSize.qml
    deadStoreLoop.qml
    dialog.qml
    dialogButtonBox.qml
    dynamicscene.qml
    enumConversion.qml
    enumFromBadSingleton.qml
    enumInvalid.qml
    enumLookup.qml
    enumMarkedAsFlag.qml
    enumProblems.qml
    enumScope.qml
    enumsInOtherObject.qml
    enumsUser.qml
    equalityQObjects.qml
    equalityQUrl.qml
    equalityTestsWithNullOrUndefined.qml
    equalityVarAndNonStorable.qml
    equalsUndefined.qml
    exceptionFromInner.qml
    excessiveParameters.qml
    extendedTypes.qml
    failures.qml
    fallbacklookups.qml
    fileDialog.qml
    flagEnum.qml
    fromBoolValue.qml
    funcWithParams.qml
    functionLookup.qml
    functionReturningVoid.qml
    functionTakingVar.qml
    globals.qml
    hidden/Main.qml
    hidden/Style.qml
    idAccess.qml
    immediateQuit.qml
    imports/QmlBench/Globals.qml
    importsFromImportPath.qml
    infinities.qml
    infinitiesToInt.qml
    intEnumCompare.qml
    intOverflow.qml
    intToEnum.qml
    interactive.qml
    interceptor.qml
    invisibleBase.qml
    invisibleListElementType.qml
    invisibleTypes.qml
    isnan.qml
    javaScriptArgument.qml
    jsArrayMethods.qml
    jsArrayMethodsUntyped.qml
    jsArrayMethodsWithParams.qml
    jsArrayMethodsWithParamsUntyped.qml
    jsMathObject.qml
    jsimport.qml
    jsmoduleimport.qml
    layouts.qml
    letAndConst.qml
    library.js
    listAsArgument.qml
    listConversion.qml
    listIndices.qml
    listOfInvisible.qml
    listPropertyAsModel.qml
    listlength.qml
    math.qml
    mathMinMax.qml
    mathOperations.qml
    mergedObjectRead.qml
    mergedObjectWrite.qml
    methods.qml
    modulePrefix.qml
    moveRegVoid.qml
    multiforeign.qml
    multipleCtors.qml
    namespaceWithEnum.qml
    noBindingLoop.qml
    noQQmlData.qml
    nonNotifyable.qml
    noscope.qml
    notEqualsInt.qml
    notNotString.qml
    nullAccess.qml
    nullComparison.qml
    numbersInJsPrimitive.qml
    objectInVar.qml
    objectLookupOnListElement.qml
    objectWithStringListMethod.qml
    outOfBounds.qml
    overriddenMember.qml
    ownProperty.qml
    page.qml
    parentProp.qml
    popContextAfterRet.qml
    prefixedMetaType.qml
    pressAndHoldButton.qml
    readEnumFromInstance.qml
    registerPropagation.qml
    registerelimination.qml
    revisions.qml
    scopeVsObject.qml
    script.js
    script.mjs
    sequenceToIterable.qml
    setLookupConversion.qml
    shadowedAsCasts.qml
    shadowedMethod.qml
    shared/Slider.qml
    shifts.qml
    signal.qml
    signalHandler.qml
    signalIndexMismatch.qml
    signalsWithLists.qml
    signatureIgnored.qml
    specificParent.qml
    storeElementSideEffects.qml
    stringArg.qml
    stringLength.qml
    stringToByteArray.qml
    testlogger.js
    text.qml
    themerbad.qml
    themergood.qml
    thisObject.qml
    throwObjectName.qml
    toString.qml
    topLevelComponent.qml
    translation.qml
    trivialSignalHandler.qml
    typePropagationLoop.qml
    typePropertyClash.qml
    typedArray.qml
    undefinedResets.qml
    undefinedToDouble.qml
    unknownAttached.qml
    unknownParameter.qml
    unstoredUndefined.qml
    unusedAttached.qml
    urlString.qml
    usingCxxTypesFromFileImports.qml
    valueTypeCast.qml
    valueTypeCopy.qml
    valueTypeDefault.qml
    valueTypeLists.qml
    valueTypeProperty.qml
    valueTypeReference.qml
    variantMapLookup.qml
    variantReturn.qml
    variantlist.qml
    versionmismatch.qml
    voidConversion.qml
    voidfunction.qml
    dummy_imports.qml
)

set(resource_files
    ProgressBar/built-with-Qt_Large.png
    imports/QmlBench/qmldir
)

set_source_files_properties("shared/Slider.qml"
    PROPERTIES QT_RESOURCE_ALIAS "Slider.qml"
)

set_source_files_properties("hidden/Style.qml"
    PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_library(codegen_test_module STATIC)
qt_autogen_tools_initial_setup(codegen_test_module)

set_target_properties(codegen_test_module PROPERTIES
    # We really want qmlcachegen here, even if qmlsc is available
    QT_QMLCACHEGEN_EXECUTABLE qmlcachegen
)

qt_policy(SET QTP0001 NEW)

qt6_add_qml_module(codegen_test_module
    VERSION 1.5
    URI TestTypes
    IMPORT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/imports/"
    DEPENDENCIES
        QtQuick
        QtQuick.Templates
        QtQuick.Shapes
    SOURCES
        ${cpp_sources}
    QML_FILES
        ${qml_files}
    RESOURCES
        ${resource_files}
    OUTPUT_DIRECTORY TestTypes # Make sure tst_qmlcachegen doesn't see our output
)

add_dependencies(codegen_test_module Qt::Quick Qt::QuickTemplates2 Qt::QuickShapesPrivate)

qt_autogen_tools_initial_setup(codegen_test_moduleplugin)


qt_add_library(codegen_test_module_verify STATIC)
qt_autogen_tools_initial_setup(codegen_test_module_verify)

set_target_properties(codegen_test_module_verify PROPERTIES
    # We really want qmlcachegen here, even if qmlsc is available
    QT_QMLCACHEGEN_EXECUTABLE qmlcachegen
)


qt6_add_qml_module(codegen_test_module_verify
    VERSION 1.5
    URI TestTypes
    IMPORT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/imports/"
    DEPENDENCIES
        QtQuick
        QtQuick.Templates
        QtQuick.Shapes
    SOURCES
        ${cpp_sources}
    QML_FILES
        ${qml_files}
    RESOURCES
        ${resource_files}
    OUTPUT_DIRECTORY verify/TestTypes # Make sure tst_qmlcachegen doesn't see our output
    TARGET_PATH verify/TestTypes      # Different target path to avoid resource file name clashes
)

add_dependencies(codegen_test_module_verify Qt::Quick Qt::QuickTemplates2 Qt::QuickShapesPrivate)

qt_autogen_tools_initial_setup(codegen_test_module_verifyplugin)


qt_internal_add_test(tst_qmlcppcodegen_verify
    SOURCES
        tst_qmlcppcodegen_verify.cpp
)

add_dependencies(tst_qmlcppcodegen_verify codegen_test_module codegen_test_module_verify)

set(a_files "")
set(b_files "")

foreach(qml_file IN LISTS qml_files)
    string(REGEX REPLACE "\\.(js|mjs|qml)$" "_\\1" compiled_file ${qml_file})
    string(REGEX REPLACE "[$#?]+" "_" compiled_file ${compiled_file})

    list(APPEND
        a_files
        "${CMAKE_CURRENT_BINARY_DIR}/.rcc/qmlcache/codegen_test_module_${compiled_file}.cpp")

    list(APPEND
        b_files
        "${CMAKE_CURRENT_BINARY_DIR}/.rcc/qmlcache/codegen_test_module_verify_${compiled_file}.cpp")
endforeach()

qt_add_resources(tst_qmlcppcodegen_verify "a"
    PREFIX
        "/a"
    FILES
        ${a_files}
    BASE
        "${CMAKE_CURRENT_BINARY_DIR}/.rcc/qmlcache/"
)

qt_add_resources(tst_qmlcppcodegen_verify "b"
    PREFIX
        "/b"
    FILES
        ${b_files}
    BASE
        "${CMAKE_CURRENT_BINARY_DIR}/.rcc/qmlcache/"
)
