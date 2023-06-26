# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_example(quick-accessibility)
qt_internal_add_example(animation)
qt_internal_add_example(draganddrop)
qt_internal_add_example(canvas)
qt_internal_add_example(imageelements)
qt_internal_add_example(keyinteraction)
qt_internal_add_example(layouts)
qt_internal_add_example(responsivelayouts)
qt_internal_add_example(localstorage)
add_subdirectory(models)
qt_internal_add_example(views)
add_subdirectory(tableview)
qt_internal_add_example(mousearea)
qt_internal_add_example(positioners)
add_subdirectory(scenegraph)
qt_internal_add_example(shadereffects)
qt_internal_add_example(text)
add_subdirectory(tutorials)
add_subdirectory(customitems)
qt_internal_add_example(imageprovider)
qt_internal_add_example(imageresponseprovider)
qt_internal_add_example(window)
add_subdirectory(particles)
qt_internal_add_example(delegatechooser)
qt_internal_add_example(shapes)
qt_internal_add_example(itemvariablerefreshrate)
add_subdirectory(multieffect)
if(QT_FEATURE_opengl OR QT_FEATURE_opengles2 OR QT_FEATURE_opengles3)
    add_subdirectory(rendercontrol)
endif()
if(TARGET Qt6::Widgets)
    qt_internal_add_example(embeddedinwidgets)
endif()
if(TARGET Qt6::QuickWidgets AND TARGET Qt6::Widgets AND (QT_FEATURE_opengl OR QT_FEATURE_opengles2 OR QT_FEATURE_opengles3))
    add_subdirectory(quickwidgets)
endif()

# qt_examples_build_end() misses at least some of these due to some
# source subdirectories being added multiple times. See QTBUG-96159.
set(reused_dir_targets
    animation_shared
    canvas_shared
    views_shared
    positioners_shared
    text_shared
    window_shared
    shapes_shared
    imageelements_shared
    delegatechooser_shared
    pointerhandlers_shared
    affectors_shared
    emitters_shared
    imageparticle_shared
    itemparticle_shared
    system_shared
    draganddrop_shared
    testbed_shared
    itemswitcher_shared
)

foreach(target IN LISTS reused_dir_targets)
    if(TARGET ${target})
        qt_autogen_tools(${target} ENABLE_AUTOGEN_TOOLS moc rcc)
        if(TARGET Qt6::Widgets)
            qt_autogen_tools(${target} ENABLE_AUTOGEN_TOOLS uic)
        endif()
    endif()
endforeach()
