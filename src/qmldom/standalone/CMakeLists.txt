cmake_minimum_required(VERSION 3.16)

project(standalone_qmldom LANGUAGES CXX)

find_package(Qt6 COMPONENTS Core Qml)

include(../../../.cmake.conf)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
elseif (MINGW)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wa,-mbig-obj")
endif()


add_library(qmldomlib
        STATIC
        ../../qmlcompiler/qcoloroutput.cpp ../../qmlcompiler/qcoloroutput_p.h
        ../../qmlcompiler/qdeferredpointer_p.h
        ../../qmlcompiler/qqmljsannotation.cpp  ../../qmlcompiler/qqmljsannotation_p.h
        ../../qmlcompiler/qqmljsannotation.cpp ../../qmlcompiler/qqmljsannotation_p.h
        ../../qmlcompiler/qqmljsannotation.cpp ../../qmlcompiler/qqmljsannotation_p.h
        ../../qmlcompiler/qqmljsimporter.cpp ../../qmlcompiler/qqmljsimporter_p.h
        ../../qmlcompiler/qqmljsimportvisitor.cpp ../../qmlcompiler/qqmljsimportvisitor_p.h
        ../../qmlcompiler/qqmljslogger.cpp ../../qmlcompiler/qqmljslogger_p.h
        ../../qmlcompiler/qqmljsmetatypes_p.h
        ../../qmlcompiler/qqmljsresourcefilemapper.cpp ../../qmlcompiler/qqmljsresourcefilemapper_p.h
        ../../qmlcompiler/qqmljsscope.cpp ../../qmlcompiler/qqmljsscope_p.h
        ../../qmlcompiler/qqmljstypedescriptionreader.cpp ../../qmlcompiler/qqmljstypedescriptionreader_p.h
        ../../qmlcompiler/qqmljstypereader.cpp ../../qmlcompiler/qqmljstypereader_p.h
        ../qqmldom_fwd_p.h
        ../qqmldom_global.h
        ../qqmldomastcreator.cpp ../qqmldomastcreator_p.h
        ../qqmldomastdumper.cpp ../qqmldomastdumper_p.h
        ../qqmldomattachedinfo.cpp ../qqmldomattachedinfo_p.h
        ../qqmldomcomments.cpp ../qqmldomcomments_p.h
        ../qqmldomcompare.cpp ../qqmldomcompare_p.h
        ../qqmldomconstants_p.h
        ../qqmldomelements.cpp ../qqmldomelements_p.h
        ../qqmldomerrormessage.cpp ../qqmldomerrormessage_p.h
        ../qqmldomexternalitems.cpp ../qqmldomexternalitems_p.h
        ../qqmldomfieldfilter.cpp ../qqmldomfieldfilter_p.h
        ../qqmldomfilewriter.cpp ../qqmldomfilewriter_p.h
        ../qqmldomfunctionref_p.h
        ../qqmldomitem.cpp ../qqmldomitem_p.h
        ../qqmldomlinewriter.cpp ../qqmldomlinewriter_p.h
        ../qqmldommock.cpp ../qqmldommock_p.h
        ../qqmldommoduleindex.cpp ../qqmldommoduleindex_p.h
        ../qqmldomoutwriter.cpp ../qqmldomoutwriter_p.h
        ../qqmldompath.cpp ../qqmldompath_p.h
        ../qqmldomreformatter.cpp ../qqmldomreformatter_p.h
        ../qqmldomstringdumper.cpp ../qqmldomstringdumper_p.h
        ../qqmldomtop.cpp ../qqmldomtop_p.h
        ../qqmldomtypesreader.cpp ../qqmldomtypesreader_p.h
)

target_compile_definitions(qmldomlib PUBLIC QMLDOM_STANDALONE)

target_include_directories(qmldomlib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../..)
target_link_libraries(qmldomlib
  PUBLIC
         Qt::CorePrivate
         Qt::QmlPrivate
)
