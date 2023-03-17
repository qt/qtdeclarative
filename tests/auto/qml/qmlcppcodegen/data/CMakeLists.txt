# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(cpp_sources
    ambiguous.h
    birthdayparty.cpp birthdayparty.h
    cppbaseclass.h
    druggeljug.h
    dynamicmeta.h
    enumproblems.h
    enumProperty.h
    gadgetwithenum.h
    invisible.h
    multiforeign.h
    objectwithmethod.h
    person.cpp person.h
    sequencetypeexample.cpp sequencetypeexample.h
    state.h
    theme.cpp theme.h
    timelinetheme.cpp timelinetheme.h
    variantMapLookup.h
    wrapwithvariant.h
    withlength.h
)

set(qml_files
    AccessModelMethodsFromOutside.qml
    ArraySequenceLengthInterop.qml
    BadType.qml
    BaseMember.qml
    BindingExpression.qml
    Cycle1.qml
    Cycle2.qml
    Cycle3.qml
    CxxTypeFromDir.qml
    CxxTypeFromImplicit.qml
    Dummy.qml
    Enums.qml
    Foozle.qml
    Loopy.qml
    OkType.qml
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
    bindToValueType.qml
    blockComments.qml
    boundComponents.qml
    callContextPropertyLookupResult.qml
    callWithSpread.qml
    childobject.qml
    colorAsVariant.qml
    colorString.qml
    consoleObject.qml
    componentReturnType.qml
    compositeTypeMethod.qml
    compositesingleton.qml
    construct.qml
    contextParam.qml
    conversionDecrement.qml
    conversions.qml
    conversions2.qml
    curlygrouped.qml
    cycleHead.qml
    deadShoeSize.qml
    deadStoreLoop.qml
    dialog.qml
    dialogButtonBox.qml
    dynamicscene.qml
    enumConversion.qml
    enumInvalid.qml
    enumLookup.qml
    enumProblems.qml
    enumScope.qml
    enumsInOtherObject.qml
    enumsUser.qml
    equalityQObjects.qml
    equalityQUrl.qml
    equalityVarAndNonStorable.qml
    equalsUndefined.qml
    excessiveParameters.qml
    extendedTypes.qml
    failures.qml
    fallbacklookups.qml
    fileDialog.qml
    fromBoolValue.qml
    functionLookup.qml
    funcWithParams.qml
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
    invisibleBase.qml
    invisibleTypes.qml
    invisibleListElementType.qml
    intEnumCompare.qml
    intOverflow.qml
    interactive.qml
    interceptor.qml
    isnan.qml
    javaScriptArgument.qml
    jsMathObject.qml
    jsimport.qml
    jsmoduleimport.qml
    layouts.qml
    library.js
    letAndConst.qml
    listAsArgument.qml
    listIndices.qml
    listPropertyAsModel.qml
    listlength.qml
    math.qml
    mathOperations.qml
    methods.qml
    modulePrefix.qml
    moveRegVoid.qml
    multiforeign.qml
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
    outOfBounds.qml
    overriddenMember.qml
    ownProperty.qml
    page.qml
    parentProp.qml
    popContextAfterRet.qml
    prefixedMetaType.qml
    pressAndHoldButton.qml
    registerelimination.qml
    registerPropagation.qml
    revisions.qml
    scopeVsObject.qml
    script.js
    script.mjs
    shared/Slider.qml
    shifts.qml
    signal.qml
    signalHandler.qml
    signalIndexMismatch.qml
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
    throwObjectName.qml
    toString.qml
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
    valueTypeCopy.qml
    valueTypeLists.qml
    valueTypeProperty.qml
    valueTypeReference.qml
    variantlist.qml
    variantMapLookup.qml
    versionmismatch.qml
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
