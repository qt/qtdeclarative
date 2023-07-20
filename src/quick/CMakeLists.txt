# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## Quick Module:
#####################################################################

qt_internal_add_qml_module(Quick
    URI "QtQuick"
    VERSION "${PROJECT_VERSION}"
    DESIGNER_SUPPORTED
    PLUGIN_TARGET qtquick2plugin
    NO_GENERATE_PLUGIN_SOURCE
    CLASS_NAME QtQuick2Plugin
    IMPORTS
        QtQml/auto
    PLUGIN_TYPES scenegraph
    SOURCES
        handlers/qquickdragaxis.cpp handlers/qquickdragaxis_p.h
        handlers/qquickdraghandler.cpp handlers/qquickdraghandler_p.h
        handlers/qquickhandlerpoint.cpp handlers/qquickhandlerpoint_p.h
        handlers/qquickhoverhandler.cpp handlers/qquickhoverhandler_p.h
        handlers/qquickmultipointhandler.cpp handlers/qquickmultipointhandler_p.h
        handlers/qquickmultipointhandler_p_p.h
        handlers/qquickpinchhandler.cpp handlers/qquickpinchhandler_p.h
        handlers/qquickpointerdevicehandler.cpp handlers/qquickpointerdevicehandler_p.h
        handlers/qquickpointerdevicehandler_p_p.h
        handlers/qquickpointerhandler.cpp handlers/qquickpointerhandler_p.h
        handlers/qquickpointerhandler_p_p.h
        handlers/qquickpointhandler.cpp handlers/qquickpointhandler_p.h
        handlers/qquicksinglepointhandler.cpp handlers/qquicksinglepointhandler_p.h
        handlers/qquicksinglepointhandler_p_p.h
        handlers/qquicktaphandler.cpp handlers/qquicktaphandler_p.h
        items/qquickabstractpaletteprovider_p.h
        items/qquickaccessibleattached.cpp items/qquickaccessibleattached_p.h
        items/qquickanchors.cpp items/qquickanchors_p.h
        items/qquickanchors_p_p.h
        items/qquickborderimage.cpp items/qquickborderimage_p.h
        items/qquickborderimage_p_p.h
        items/qquickclipnode.cpp items/qquickclipnode_p.h
        items/qquickcolorgroup.cpp items/qquickcolorgroup_p.h
        items/qquickevents.cpp
        items/qquickevents_p_p.h
        items/qquickflickable.cpp items/qquickflickable_p.h
        items/qquickflickable_p_p.h
        items/qquickflickablebehavior_p.h
        items/qquickfocusscope.cpp items/qquickfocusscope_p.h
        items/qquickgraphicsconfiguration.cpp items/qquickgraphicsconfiguration.h items/qquickgraphicsconfiguration_p.h
        items/qquickgraphicsdevice.cpp items/qquickgraphicsdevice.h items/qquickgraphicsdevice_p.h
        items/qquickgraphicsinfo.cpp items/qquickgraphicsinfo_p.h
        items/qquickimage.cpp items/qquickimage_p.h
        items/qquickimage_p_p.h
        items/qquickimagebase.cpp items/qquickimagebase_p.h
        items/qquickimagebase_p_p.h
        items/qquickimplicitsizeitem.cpp items/qquickimplicitsizeitem_p.h
        items/qquickimplicitsizeitem_p_p.h
        items/qquickitem.cpp items/qquickitem.h items/qquickitem_p.h
        items/qquickitemanimation.cpp items/qquickitemanimation_p.h
        items/qquickitemanimation_p_p.h
        items/qquickitemchangelistener_p.h
        items/qquickitemgrabresult.cpp items/qquickitemgrabresult.h
        items/qquickitemsmodule.cpp items/qquickitemsmodule_p.h
        items/qquickloader.cpp items/qquickloader_p.h
        items/qquickloader_p_p.h
        items/qquickmousearea.cpp items/qquickmousearea_p.h
        items/qquickmousearea_p_p.h
        items/qquickmultipointtoucharea.cpp items/qquickmultipointtoucharea_p.h
        items/qquickpainteditem.cpp items/qquickpainteditem.h items/qquickpainteditem_p.h
        items/qquickpalette.cpp items/qquickpalette_p.h
        items/qquickpalettecolorprovider.cpp items/qquickpalettecolorprovider_p.h
        items/qquickpaletteproviderprivatebase_p.h
        items/qquickpincharea.cpp items/qquickpincharea_p.h
        items/qquickpincharea_p_p.h
        items/qquickrectangle.cpp items/qquickrectangle_p.h
        items/qquickrectangle_p_p.h
        items/qquickrendercontrol.cpp items/qquickrendercontrol.h items/qquickrendercontrol_p.h
        items/qquickrendertarget.cpp items/qquickrendertarget.h items/qquickrendertarget_p.h
        items/qquickscalegrid.cpp
        items/qquickscalegrid_p_p.h
        items/qquickscreen.cpp items/qquickscreen_p.h
        items/qquickstateoperations.cpp items/qquickstateoperations_p.h
        items/qquicktext.cpp items/qquicktext_p.h
        items/qquicktext_p_p.h
        items/qquicktextinterface_p.h
        items/qquicktextcontrol.cpp items/qquicktextcontrol_p.h
        items/qquicktextcontrol_p_p.h
        items/qquicktextdocument.cpp items/qquicktextdocument.h items/qquicktextdocument_p.h
        items/qquicktextedit.cpp items/qquicktextedit_p.h
        items/qquicktextedit_p_p.h
        items/qquicktextinput.cpp items/qquicktextinput_p.h
        items/qquicktextinput_p_p.h
        items/qquicktextnode.cpp items/qquicktextnode_p.h
        items/qquicktextnodeengine.cpp items/qquicktextnodeengine_p.h
        items/qquicktextutil.cpp items/qquicktextutil_p.h
        items/qquicktranslate.cpp items/qquicktranslate_p.h
        items/qquickview.cpp items/qquickview.h items/qquickview_p.h
        items/qquickwindow.cpp items/qquickwindow.h items/qquickwindow_p.h
        items/qquickwindowattached.cpp items/qquickwindowattached_p.h
        items/qquickwindowmodule.cpp items/qquickwindowmodule_p.h
        items/qquickwindowmodule_p_p.h
        qtquickglobal.h qtquickglobal_p.h
        scenegraph/adaptations/software/qsgabstractsoftwarerenderer.cpp scenegraph/adaptations/software/qsgabstractsoftwarerenderer_p.h
        scenegraph/adaptations/software/qsgsoftwareadaptation.cpp scenegraph/adaptations/software/qsgsoftwareadaptation_p.h
        scenegraph/adaptations/software/qsgsoftwarecontext.cpp scenegraph/adaptations/software/qsgsoftwarecontext_p.h
        scenegraph/adaptations/software/qsgsoftwareglyphnode.cpp scenegraph/adaptations/software/qsgsoftwareglyphnode_p.h
        scenegraph/adaptations/software/qsgsoftwareinternalimagenode.cpp scenegraph/adaptations/software/qsgsoftwareinternalimagenode_p.h
        scenegraph/adaptations/software/qsgsoftwareinternalrectanglenode.cpp scenegraph/adaptations/software/qsgsoftwareinternalrectanglenode_p.h
        scenegraph/adaptations/software/qsgsoftwarelayer.cpp scenegraph/adaptations/software/qsgsoftwarelayer_p.h
        scenegraph/adaptations/software/qsgsoftwarepainternode.cpp scenegraph/adaptations/software/qsgsoftwarepainternode_p.h
        scenegraph/adaptations/software/qsgsoftwarepixmaprenderer.cpp scenegraph/adaptations/software/qsgsoftwarepixmaprenderer_p.h
        scenegraph/adaptations/software/qsgsoftwarepixmaptexture.cpp scenegraph/adaptations/software/qsgsoftwarepixmaptexture_p.h
        scenegraph/adaptations/software/qsgsoftwarepublicnodes.cpp scenegraph/adaptations/software/qsgsoftwarepublicnodes_p.h
        scenegraph/adaptations/software/qsgsoftwarerenderablenode.cpp scenegraph/adaptations/software/qsgsoftwarerenderablenode_p.h
        scenegraph/adaptations/software/qsgsoftwarerenderablenodeupdater.cpp scenegraph/adaptations/software/qsgsoftwarerenderablenodeupdater_p.h
        scenegraph/adaptations/software/qsgsoftwarerenderer.cpp scenegraph/adaptations/software/qsgsoftwarerenderer_p.h
        scenegraph/adaptations/software/qsgsoftwarerenderlistbuilder.cpp scenegraph/adaptations/software/qsgsoftwarerenderlistbuilder_p.h
        scenegraph/adaptations/software/qsgsoftwarerenderloop.cpp scenegraph/adaptations/software/qsgsoftwarerenderloop_p.h
        scenegraph/compressedtexture/qsgcompressedatlastexture.cpp scenegraph/compressedtexture/qsgcompressedatlastexture_p.h
        scenegraph/compressedtexture/qsgcompressedtexture.cpp scenegraph/compressedtexture/qsgcompressedtexture_p.h
        scenegraph/coreapi/qsgabstractrenderer.cpp scenegraph/coreapi/qsgabstractrenderer_p.h
        scenegraph/coreapi/qsgabstractrenderer_p_p.h
        scenegraph/coreapi/qsgbatchrenderer.cpp scenegraph/coreapi/qsgbatchrenderer_p.h
        scenegraph/coreapi/qsggeometry.cpp scenegraph/coreapi/qsggeometry.h scenegraph/coreapi/qsggeometry_p.h
        scenegraph/coreapi/qsgmaterial.cpp scenegraph/coreapi/qsgmaterial.h
        scenegraph/coreapi/qsgmaterialshader.cpp scenegraph/coreapi/qsgmaterialshader.h scenegraph/coreapi/qsgmaterialshader_p.h
        scenegraph/coreapi/qsgmaterialtype.h
        scenegraph/coreapi/qsgnode.cpp scenegraph/coreapi/qsgnode.h scenegraph/coreapi/qsgnode_p.h
        scenegraph/coreapi/qsgnodeupdater.cpp scenegraph/coreapi/qsgnodeupdater_p.h
        scenegraph/coreapi/qsgrenderer.cpp scenegraph/coreapi/qsgrenderer_p.h
        scenegraph/coreapi/qsgrendererinterface.cpp scenegraph/coreapi/qsgrendererinterface.h
        scenegraph/coreapi/qsgrendernode.cpp scenegraph/coreapi/qsgrendernode.h scenegraph/coreapi/qsgrendernode_p.h
        scenegraph/coreapi/qsgrhivisualizer.cpp scenegraph/coreapi/qsgrhivisualizer_p.h
        scenegraph/coreapi/qsgtexture.cpp scenegraph/coreapi/qsgtexture.h scenegraph/coreapi/qsgtexture_p.h
        scenegraph/coreapi/qsgtexture_platform.h
        scenegraph/qsgadaptationlayer.cpp scenegraph/qsgadaptationlayer_p.h
        scenegraph/qsgbasicglyphnode.cpp scenegraph/qsgbasicglyphnode_p.h
        scenegraph/qsgbasicinternalimagenode.cpp scenegraph/qsgbasicinternalimagenode_p.h
        scenegraph/qsgbasicinternalrectanglenode.cpp scenegraph/qsgbasicinternalrectanglenode_p.h
        scenegraph/qsgcontext.cpp scenegraph/qsgcontext_p.h
        scenegraph/qsgcontextplugin.cpp scenegraph/qsgcontextplugin_p.h
        scenegraph/qsgdefaultcontext.cpp scenegraph/qsgdefaultcontext_p.h
        scenegraph/qsgdefaultglyphnode.cpp scenegraph/qsgdefaultglyphnode_p.cpp scenegraph/qsgdefaultglyphnode_p.h
        scenegraph/qsgdefaultglyphnode_p_p.h
        scenegraph/qsgdefaultinternalimagenode.cpp scenegraph/qsgdefaultinternalimagenode_p.h
        scenegraph/qsgdefaultinternalrectanglenode.cpp scenegraph/qsgdefaultinternalrectanglenode_p.h
        scenegraph/qsgdefaultrendercontext.cpp scenegraph/qsgdefaultrendercontext_p.h
        scenegraph/qsgdistancefieldglyphnode.cpp scenegraph/qsgdistancefieldglyphnode_p.cpp scenegraph/qsgdistancefieldglyphnode_p.h
        scenegraph/qsgdistancefieldglyphnode_p_p.h
        scenegraph/qsgrenderloop.cpp scenegraph/qsgrenderloop_p.h
        scenegraph/qsgrhidistancefieldglyphcache.cpp scenegraph/qsgrhidistancefieldglyphcache_p.h
        scenegraph/qsgrhilayer.cpp scenegraph/qsgrhilayer_p.h
        scenegraph/qsgrhishadereffectnode.cpp scenegraph/qsgrhishadereffectnode_p.h
        scenegraph/qsgrhisupport.cpp scenegraph/qsgrhisupport_p.h
        scenegraph/qsgrhitextureglyphcache.cpp scenegraph/qsgrhitextureglyphcache_p.h
        scenegraph/util/qsgareaallocator.cpp scenegraph/util/qsgareaallocator_p.h
        scenegraph/util/qsgdefaultimagenode.cpp scenegraph/util/qsgdefaultimagenode_p.h
        scenegraph/util/qsgdefaultninepatchnode.cpp scenegraph/util/qsgdefaultninepatchnode_p.h
        scenegraph/util/qsgdefaultpainternode.cpp scenegraph/util/qsgdefaultpainternode_p.h
        scenegraph/util/qsgdefaultrectanglenode.cpp scenegraph/util/qsgdefaultrectanglenode_p.h
        scenegraph/util/qsgflatcolormaterial.cpp scenegraph/util/qsgflatcolormaterial.h
        scenegraph/util/qsgimagenode.cpp scenegraph/util/qsgimagenode.h
        scenegraph/util/qsgninepatchnode.cpp scenegraph/util/qsgninepatchnode.h
        scenegraph/util/qsgplaintexture.cpp scenegraph/util/qsgplaintexture_p.h
        scenegraph/util/qsgrectanglenode.cpp scenegraph/util/qsgrectanglenode.h
        scenegraph/util/qsgrhiatlastexture.cpp scenegraph/util/qsgrhiatlastexture_p.h
        scenegraph/util/qsgsimplerectnode.cpp scenegraph/util/qsgsimplerectnode.h
        scenegraph/util/qsgsimpletexturenode.cpp scenegraph/util/qsgsimpletexturenode.h
        scenegraph/util/qsgtexturematerial.cpp scenegraph/util/qsgtexturematerial.h scenegraph/util/qsgtexturematerial_p.h
        scenegraph/util/qsgtextureprovider.cpp scenegraph/util/qsgtextureprovider.h
        scenegraph/util/qsgtexturereader.cpp scenegraph/util/qsgtexturereader_p.h
        scenegraph/util/qsgvertexcolormaterial.cpp scenegraph/util/qsgvertexcolormaterial.h
        util/qminimalflatset_p.h
        util/qquickanimation.cpp util/qquickanimation_p.h
        util/qquickanimation_p_p.h
        util/qquickanimationcontroller.cpp util/qquickanimationcontroller_p.h
        util/qquickanimator.cpp util/qquickanimator_p.h
        util/qquickanimator_p_p.h
        util/qquickanimatorcontroller.cpp util/qquickanimatorcontroller_p.h
        util/qquickanimatorjob.cpp util/qquickanimatorjob_p.h
        util/qquickapplication.cpp util/qquickapplication_p.h
        util/qquickbehavior.cpp util/qquickbehavior_p.h
        util/qquickdeliveryagent.cpp util/qquickdeliveryagent_p.h util/qquickdeliveryagent_p_p.h
        util/qquickfontloader.cpp util/qquickfontloader_p.h
        util/qquickfontmetrics.cpp util/qquickfontmetrics_p.h
        util/qquickforeignutils.cpp util/qquickforeignutils_p.h
        util/qquickglobal.cpp
        util/qquickimageprovider.cpp util/qquickimageprovider.h util/qquickimageprovider_p.h
        util/qquickpixmapcache.cpp util/qquickpixmapcache_p.h
        util/qquickprofiler_p.h
        util/qquickpropertychanges.cpp util/qquickpropertychanges_p.h
        util/qquicksmoothedanimation.cpp util/qquicksmoothedanimation_p.h
        util/qquicksmoothedanimation_p_p.h
        util/qquickspringanimation.cpp util/qquickspringanimation_p.h
        util/qquickstate.cpp util/qquickstate_p.h
        util/qquickstate_p_p.h
        util/qquickstatechangescript.cpp util/qquickstatechangescript_p.h
        util/qquickstategroup.cpp util/qquickstategroup_p.h
        util/qquickstyledtext.cpp util/qquickstyledtext_p.h
        util/qquicksvgparser.cpp util/qquicksvgparser_p.h
        util/qquicksystempalette.cpp util/qquicksystempalette_p.h
        util/qquicktextmetrics.cpp util/qquicktextmetrics_p.h
        util/qquicktimeline.cpp
        util/qquicktimeline_p_p.h
        util/qquicktransition.cpp util/qquicktransition_p.h
        util/qquicktransitionmanager.cpp
        util/qquicktransitionmanager_p_p.h
        util/qquickvalidator.cpp util/qquickvalidator_p.h
        util/qquickvaluetypes.cpp util/qquickvaluetypes_p.h
        util/qquickframeanimation.cpp util/qquickframeanimation_p.h
    NO_UNITY_BUILD_SOURCES
        scenegraph/qsgdefaultcontext.cpp # redefinition of 'qmlDisableDistanceField' (from qquicktextinput.cpp)
        scenegraph/util/qsgtexturematerial.cpp # redefinition of 'isPowerOfTwo' (from qsgdefaultinternalimagenode.cpp)
        util/qquickvalidator.cpp # expl. spec. of 'QMetaTypeId<QValidator *>' after inst.
        quick_qmltyperegistrations.cpp # qquickvalidator_p.h: error: explicit specialization of 'QMetaTypeId<QValidator *>' after instantiation
    DEFINES
        QT_NO_FOREACH
        QT_NO_INTEGER_EVENT_COORDINATES
        QT_NO_URL_CAST_FROM_STRING
    INCLUDE_DIRECTORIES
        ${CMAKE_CURRENT_SOURCE_DIR}
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlModelsPrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::QmlModels
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlModelsPrivate
        Qt::QmlPrivate
    GENERATE_CPP_EXPORTS
    GENERATE_PRIVATE_CPP_EXPORTS
)

# We need to do additional initialization, so we have to provide our own
# plugin class rather than using the generated one
qt_internal_extend_target(qtquick2plugin
    SOURCES   qtquickplugin.cpp
    LIBRARIES Qt::QuickPrivate
)

qt_internal_add_shaders(Quick "scenegraph_shaders"
    SILENT
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    PREFIX
        "/qt-project.org"
    FILES
        "scenegraph/shaders_ng/24bittextmask.frag"
        "scenegraph/shaders_ng/32bitcolortext.frag"
        "scenegraph/shaders_ng/8bittextmask.frag"
        "scenegraph/shaders_ng/8bittextmask_a.frag"
        "scenegraph/shaders_ng/distancefieldoutlinetext.frag"
        "scenegraph/shaders_ng/distancefieldoutlinetext.vert"
        "scenegraph/shaders_ng/distancefieldoutlinetext_a.frag"
        "scenegraph/shaders_ng/distancefieldoutlinetext_a_fwidth.frag"
        "scenegraph/shaders_ng/distancefieldoutlinetext_fwidth.frag"
        "scenegraph/shaders_ng/distancefieldshiftedtext.frag"
        "scenegraph/shaders_ng/distancefieldshiftedtext.vert"
        "scenegraph/shaders_ng/distancefieldshiftedtext_a.frag"
        "scenegraph/shaders_ng/distancefieldshiftedtext_a_fwidth.frag"
        "scenegraph/shaders_ng/distancefieldshiftedtext_fwidth.frag"
        "scenegraph/shaders_ng/distancefieldtext.frag"
        "scenegraph/shaders_ng/distancefieldtext.vert"
        "scenegraph/shaders_ng/distancefieldtext_a.frag"
        "scenegraph/shaders_ng/distancefieldtext_a_fwidth.frag"
        "scenegraph/shaders_ng/distancefieldtext_fwidth.frag"
        "scenegraph/shaders_ng/flatcolor.frag"
        "scenegraph/shaders_ng/flatcolor.vert"
        "scenegraph/shaders_ng/hiqsubpixeldistancefieldtext.frag"
        "scenegraph/shaders_ng/hiqsubpixeldistancefieldtext.vert"
        "scenegraph/shaders_ng/hiqsubpixeldistancefieldtext_a.frag"
        "scenegraph/shaders_ng/loqsubpixeldistancefieldtext.frag"
        "scenegraph/shaders_ng/loqsubpixeldistancefieldtext.vert"
        "scenegraph/shaders_ng/loqsubpixeldistancefieldtext_a.frag"
        "scenegraph/shaders_ng/opaquetexture.frag"
        "scenegraph/shaders_ng/opaquetexture.vert"
        "scenegraph/shaders_ng/outlinedtext.frag"
        "scenegraph/shaders_ng/outlinedtext.vert"
        "scenegraph/shaders_ng/outlinedtext_a.frag"
        "scenegraph/shaders_ng/shadereffect.frag"
        "scenegraph/shaders_ng/shadereffect.vert"
        "scenegraph/shaders_ng/smoothcolor.frag"
        "scenegraph/shaders_ng/smoothcolor.vert"
        "scenegraph/shaders_ng/smoothtexture.frag"
        "scenegraph/shaders_ng/smoothtexture.vert"
        "scenegraph/shaders_ng/sprite.frag"
        "scenegraph/shaders_ng/sprite.vert"
        "scenegraph/shaders_ng/stencilclip.frag"
        "scenegraph/shaders_ng/stencilclip.vert"
        "scenegraph/shaders_ng/styledtext.frag"
        "scenegraph/shaders_ng/styledtext.vert"
        "scenegraph/shaders_ng/styledtext_a.frag"
        "scenegraph/shaders_ng/textmask.frag"
        "scenegraph/shaders_ng/textmask.vert"
        "scenegraph/shaders_ng/texture.frag"
        "scenegraph/shaders_ng/texture.vert"
        "scenegraph/shaders_ng/vertexcolor.frag"
        "scenegraph/shaders_ng/vertexcolor.vert"
        "scenegraph/shaders_ng/visualization.frag"
        "scenegraph/shaders_ng/visualization.vert"
)

qt_internal_extend_target(Quick CONDITION QT_FEATURE_qml_network
    LIBRARIES
        Qt::Network
)

qt_internal_extend_target(Quick CONDITION QT_FEATURE_opengl
    LIBRARIES
        Qt::OpenGLPrivate
    PUBLIC_LIBRARIES
        Qt::OpenGL
)

qt_internal_extend_target(Quick CONDITION MSVC
    DEFINES
        _CRT_SECURE_NO_WARNINGS
)

#### Keys ignored in scope 5:.:.:quick.pro:solaris-cc_x_:
# QMAKE_CXXFLAGS_RELEASE = "--O2"

qt_internal_extend_target(Quick CONDITION WIN32
    PUBLIC_LIBRARIES
        user32
)

qt_internal_extend_target(Quick CONDITION EXISTS "qqml_enable_gcov"
    LIBRARIES
        gcov
    COMPILE_OPTIONS
        -fno-elide-constructors
        -fprofile-arcs
        -ftest-coverage
)

qt_internal_extend_target(Quick CONDITION QT_FEATURE_quick_designer
    SOURCES
        designer/qqmldesignermetaobject.cpp designer/qqmldesignermetaobject_p.h
        designer/qquickdesignercustomobjectdata.cpp designer/qquickdesignercustomobjectdata_p.h
        designer/qquickdesignercustomparserobject.cpp designer/qquickdesignercustomparserobject_p.h
        designer/qquickdesignersupport.cpp designer/qquickdesignersupport_p.h
        designer/qquickdesignersupportitems.cpp designer/qquickdesignersupportitems_p.h
        designer/qquickdesignersupportmetainfo.cpp designer/qquickdesignersupportmetainfo_p.h
        designer/qquickdesignersupportproperties.cpp designer/qquickdesignersupportproperties_p.h
        designer/qquickdesignersupportpropertychanges.cpp designer/qquickdesignersupportpropertychanges_p.h
        designer/qquickdesignersupportstates.cpp designer/qquickdesignersupportstates_p.h
)

qt_internal_extend_target(Quick CONDITION QT_FEATURE_accessibility
    SOURCES
        accessible/qaccessiblequickitem.cpp accessible/qaccessiblequickitem_p.h
        accessible/qaccessiblequickview.cpp accessible/qaccessiblequickview_p.h
        accessible/qquickaccessiblefactory.cpp accessible/qquickaccessiblefactory_p.h
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
)

qt_internal_extend_target(Quick CONDITION QT_FEATURE_qml_debug
    SOURCES
        util/qquickprofiler.cpp
)

qt_internal_extend_target(Quick CONDITION QT_FEATURE_shortcut
    SOURCES
        util/qquickshortcut.cpp util/qquickshortcut_p.h
)

qt_internal_extend_target(Quick CONDITION QT_FEATURE_quick_path
    SOURCES
        util/qquickpath.cpp util/qquickpath_p.h
        util/qquickpath_p_p.h
        util/qquickpathinterpolator.cpp util/qquickpathinterpolator_p.h
)

qt_internal_extend_target(Quick CONDITION QT_FEATURE_opengl OR QT_FEATURE_opengles2 OR QT_FEATURE_opengles3
    SOURCES
        items/qquickframebufferobject.cpp items/qquickframebufferobject.h
        util/qquickopenglutils.cpp util/qquickopenglutils.h
)

qt_internal_extend_target(Quick CONDITION IOS OR MACOS
    SOURCES
        scenegraph/coreapi/qsgtexture_mac.mm
        scenegraph/qsgrhisupport_mac.mm
)

qt_internal_extend_target(Quick CONDITION QT_FEATURE_thread
    SOURCES
        scenegraph/adaptations/software/qsgsoftwarethreadedrenderloop.cpp scenegraph/adaptations/software/qsgsoftwarethreadedrenderloop_p.h
        scenegraph/qsgthreadedrenderloop.cpp scenegraph/qsgthreadedrenderloop_p.h
)

qt_internal_extend_target(Quick CONDITION QT_FEATURE_quick_sprite
    SOURCES
        items/qquickanimatedsprite.cpp items/qquickanimatedsprite_p.h
        items/qquickanimatedsprite_p_p.h
        items/qquicksprite.cpp items/qquicksprite_p.h
        items/qquickspriteengine.cpp items/qquickspriteengine_p.h
        items/qquickspritesequence.cpp items/qquickspritesequence_p.h
        items/qquickspritesequence_p_p.h
        scenegraph/adaptations/software/qsgsoftwarespritenode.cpp scenegraph/adaptations/software/qsgsoftwarespritenode_p.h
        scenegraph/qsgdefaultspritenode.cpp scenegraph/qsgdefaultspritenode_p.h
)

qt_internal_extend_target(Quick CONDITION QT_FEATURE_quick_draganddrop
    SOURCES
        items/qquickdrag.cpp items/qquickdrag_p.h items/qquickdrag_p_p.h
        items/qquickdroparea.cpp items/qquickdroparea_p.h
)

qt_internal_extend_target(Quick CONDITION QT_FEATURE_quick_animatedimage
    SOURCES
        items/qquickanimatedimage.cpp items/qquickanimatedimage_p.h
        items/qquickanimatedimage_p_p.h
)

qt_internal_extend_target(Quick CONDITION QT_FEATURE_quick_gridview
    SOURCES
        items/qquickgridview.cpp items/qquickgridview_p.h
)

qt_internal_extend_target(Quick CONDITION QT_FEATURE_quick_itemview
    SOURCES
        items/qquickitemview.cpp items/qquickitemview_p.h
        items/qquickitemview_p_p.h
        items/qquickitemviewfxitem.cpp
        items/qquickitemviewfxitem_p_p.h
)

qt_internal_extend_target(Quick CONDITION QT_FEATURE_quick_viewtransitions
    SOURCES
        items/qquickitemviewtransition.cpp items/qquickitemviewtransition_p.h
)

qt_internal_extend_target(Quick CONDITION QT_FEATURE_quick_listview
    SOURCES
        items/qquicklistview.cpp items/qquicklistview_p.h
)

qt_internal_extend_target(Quick CONDITION QT_FEATURE_quick_tableview
    SOURCES
        items/qquicktableview.cpp items/qquicktableview_p.h
        items/qquicktableview_p_p.h
        items/qquickselectable_p.h
)

qt_internal_extend_target(Quick CONDITION QT_FEATURE_quick_treeview
    SOURCES
        items/qquicktreeview.cpp items/qquicktreeview_p.h
        items/qquicktreeview_p_p.h
)

qt_internal_extend_target(Quick CONDITION QT_FEATURE_quick_pathview
    SOURCES
        items/qquickpathview.cpp items/qquickpathview_p.h
        items/qquickpathview_p_p.h
)

qt_internal_extend_target(Quick CONDITION QT_FEATURE_quick_positioners
    SOURCES
        items/qquickpositioners.cpp items/qquickpositioners_p.h
        items/qquickpositioners_p_p.h
    NO_UNITY_BUILD_SOURCES
        items/qquickpositioners.cpp # redefinition of 'watchedChanges' (from qquickloader.cpp)
)

qt_internal_extend_target(Quick CONDITION QT_FEATURE_quick_flipable
    SOURCES
        items/qquickflipable.cpp items/qquickflipable_p.h
)

qt_internal_extend_target(Quick CONDITION QT_FEATURE_quick_repeater
    SOURCES
        items/qquickrepeater.cpp items/qquickrepeater_p.h
        items/qquickrepeater_p_p.h
)

qt_internal_extend_target(Quick CONDITION QT_FEATURE_quick_shadereffect
    SOURCES
        items/qquickshadereffect.cpp items/qquickshadereffect_p.h items/qquickshadereffect_p_p.h
        items/qquickshadereffectmesh.cpp items/qquickshadereffectmesh_p.h
        items/qquickshadereffectsource.cpp items/qquickshadereffectsource_p.h
)

qt_internal_extend_target(Quick CONDITION QT_FEATURE_quick_canvas
    SOURCES
        items/context2d/qquickcanvascontext.cpp items/context2d/qquickcanvascontext_p.h
        items/context2d/qquickcanvasitem.cpp items/context2d/qquickcanvasitem_p.h
        items/context2d/qquickcontext2d.cpp items/context2d/qquickcontext2d_p.h
        items/context2d/qquickcontext2dcommandbuffer.cpp items/context2d/qquickcontext2dcommandbuffer_p.h
        items/context2d/qquickcontext2dtexture.cpp items/context2d/qquickcontext2dtexture_p.h
        items/context2d/qquickcontext2dtile.cpp items/context2d/qquickcontext2dtile_p.h
)

qt_internal_extend_target(Quick CONDITION QT_FEATURE_wheelevent
    SOURCES
        handlers/qquickwheelhandler.cpp handlers/qquickwheelhandler_p.h
        handlers/qquickwheelhandler_p_p.h
)

qt_internal_extend_target(Quick CONDITION QT_FEATURE_im
    SOURCES
        util/qquickinputmethod.cpp util/qquickinputmethod_p.h
)

qt_internal_generate_tracepoints(Quick quick
    SOURCES
        scenegraph/qsgrenderloop.cpp
        scenegraph/coreapi/qsgrenderer.cpp
        scenegraph/qsgthreadedrenderloop.cpp
        scenegraph/adaptations/software/qsgsoftwarethreadedrenderloop.cpp
        scenegraph/qsgadaptationlayer.cpp
)
qt_internal_add_docs(Quick
    doc/qtquick.qdocconf
)

add_subdirectory(window)
