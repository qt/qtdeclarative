# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

set(cpp_sources
    # attached types:
    cpptypes/testattachedtype.h cpptypes/testattachedtype.cpp
    # grouped types:
    cpptypes/testgroupedtype.h cpptypes/testgroupedtype.cpp
    # private properties:
    cpptypes/testprivateproperty.h cpptypes/testprivateproperty.cpp
    cpptypes/private/testprivateproperty_p.h

    cpptypes/typewithproperties.h cpptypes/typewithproperties.cpp
    # deferred:
    cpptypes/deferredpropertytypes.h cpptypes/deferredpropertytypes.cpp
    # extension types:
    cpptypes/extensiontypes.h cpptypes/extensiontypes.cpp

    cpptypes/typewithspecialproperties.h

    cpptypes/typewithmanyproperties.h
    cpptypes/singletontype.h cpptypes/singletontype.cpp
    cpptypes/typewithnamespace.h cpptypes/typewithnamespace.cpp
    cpptypes/typewithsignal.h
)

set(qml_sources
    HelloWorld.qml
    NameConflict.qml
    simpleQtQuickTypes.qml
    typeWithEnums.qml
    methods.qml
    properties.qml
    ObjectWithId.qml
    documentWithIds.qml
    importNamespace.qml
    gradients.qml
    qjsvalueAssignments.qml
    QmlTypeWithExtension.qml
    QmlTypeWithBaseTypeExtension.qml
    extensionTypeBindings.qml
    nonStandardInclude.qml
    specialProperties.qml
    regexpBindings.qml
    AliasBase.qml
    aliasAssignments.qml
    Connections.qml

    qtbug103956/SubComponent.qml
    qtbug103956/MainComponent.qml
    qtbug103956/qtbug103956_main.qml

    signalHandlers.qml
    javaScriptFunctions.qml
    changingBindings.qml
    propertyAlias.qml
    propertyAliasAttributes.qml
    propertyAlias_external.qml
    propertyChangeHandler.qml
    NestedHelloWorld.qml
    ComponentHelloWorld.qml
    listProperty.qml
    listPropertySameName.qml
    defaultProperty.qml
    defaultPropertyCorrectSelection.qml
    defaultAlias.qml
    propertyReturningFunction.qml
    AttachedProperty.qml
    attachedPropertyDerived.qml
    groupedProperty.qml
    groupedProperty_qquicktext.qml
    localImport.qml
    localImport_explicit.qml
    newPropertyBoundToOld.qml
    oldPropertyBoundToNew.qml
    nonLocalQmlPropertyBoundToAny.qml
    localDerived.qml
    justAnimation.qml
    justAnimationOnAlias.qml
    behaviorAndAnimation.qml
    behaviorAndAnimationOnAlias.qml
    singletonUser.qml
    bindingsThroughIds.qml
    localImport_context.qml
    neighbors_context.qml
    delegate_context.qml
    nontrivial_context.qml
    javascriptCaller.qml
    listView.qml
    bindingOnValueType.qml
    keyEvents.qml
    complexAliases.qml
    PrivateProperty.qml
    privatePropertySubclass.qml
    calqlatrBits.qml
    propertyChangeAndSignalHandlers.qml
    deferredProperties.qml
    deferredProperties_group.qml
    deferredProperties_attached.qml
    deferredProperties_complex.qml
    valueTypeListProperty.qml
    translations.qml
    translationsById.qml
    generalizedGroupedProperty.qml
    appendToQQmlListProperty.qml
    inlineComponents.qml
    repeaterCrash.qml
    aliases.qml
    inlineComponentsFromDifferentFiles.qml
    singletons.qml
    mySignals.qml
    myCheckBox.qml

    # support types:
    DefaultPropertySingleChild.qml
    DefaultPropertyAliasChild.qml
    DefaultPropertyManyChildren.qml
    LocallyImported.qml
    LocalWithOnCompleted.qml
    LocallyImported_context.qml
    ComponentWithAlias1.qml
    ComponentWithAlias2.qml
    ComponentWithAlias3.qml
    SingletonThing.qml
    InlineComponentProvider.qml
    InlineComponentReexporter.qml
    NamespacedTypes.qml

    badFile.qml
)

set(js_sources
    subfolder/code.js
)

set(common_libraries
    Qt::Core
    Qt::QmlPrivate
    Qt::QuickPrivate
    Qt::QuickTemplates2Private
    Qt::TestPrivate
    Qt::Gui # QColor, QMatrix4x4, ...
)

set_source_files_properties(NameConflict.qml PROPERTIES
    QT_QMLTC_FILE_BASENAME ResolvedNameConflict)

set_source_files_properties(badFile.qml PROPERTIES QT_QML_SKIP_TYPE_COMPILER TRUE)

set_source_files_properties(SingletonThing.qml PROPERTIES
   QT_QML_SINGLETON_TYPE true)

qt_add_library(qmltc_test_module STATIC)
qt_autogen_tools_initial_setup(qmltc_test_module)

# use PUBLIC everywhere to simplify the build of the test binary
target_include_directories(qmltc_test_module PUBLIC cpptypes/)
target_link_libraries(qmltc_test_module PUBLIC ${common_libraries})

# Resources:
set(qmake_immediate_resource_files
    "translations_ge.qm"
    "translationsById_ge.qm"
)
qt_internal_add_resource(qmltc_test_module "qmake_immediate"
    PREFIX
        "/i18n"
    FILES
        ${qmake_immediate_resource_files}
)

qt_policy(SET QTP0001 NEW)

qt6_add_qml_module(qmltc_test_module
    VERSION 1.0
    URI QmltcTests
    SOURCES
        ${cpp_sources}
    QML_FILES
        ${qml_sources}
        ${js_sources}
    DEPENDENCIES
        QtQuick
    LIBRARIES
        qmltc_test_module_translation_by_id
    ENABLE_TYPE_COMPILER
)

qt_autogen_tools_initial_setup(qmltc_test_moduleplugin)
