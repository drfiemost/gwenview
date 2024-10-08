cmake_minimum_required(VERSION 3.5)
project(gwenview)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR
        "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options("$<$<CONFIG:DEBUG>:-Wall;-Wextra;-pedantic>")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )

find_package(KDE4 4.6.41 REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)
include(MacroOptionalAddSubdirectory)

KDE4_NO_ENABLE_FINAL(gwenview)

## Dependencies
find_package(JPEG)
macro_log_feature(JPEG_FOUND "libjpeg" "JPEG image manipulation support" "http://libjpeg.sourceforge.net/" TRUE)

find_package(PNG)
macro_log_feature(PNG_FOUND "libpng" "PNG image manipulation support" "http://www.libpng.org" TRUE)

find_package(exiv2)
macro_log_feature(exiv2_FOUND "exiv2" "Image metadata support" "http://www.exiv2.org" TRUE "0.27")
if(exiv2_FOUND)
    set(EXIV2_LIBRARIES exiv2)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")
endif()

set(FPHSA_NAME_MISMATCHED TRUE)
macro_optional_find_package(Kipi)
macro_log_feature(KIPI_FOUND "libkipi" "Provides various image manipulation and export features" "http://www.kipi-plugins.org" FALSE)

find_package(LCMS2)
macro_log_feature(LCMS2_FOUND "LittleCMS" "Color management engine" "http://www.littlecms.com" TRUE "2.0")

set(FPHSA_NAME_MISMATCHED TRUE)
find_package(LibKonq)
macro_log_feature(LIBKONQ_FOUND "libkonq" "Standard file management features" "http://www.kde.org" TRUE)

find_package(Kdcraw 2.4.2)
macro_log_feature(KDCRAW_FOUND "libkdcraw" "KDE Dcraw library" "http://www.kde.org" TRUE "2.4.2")

## Global settings

add_definitions(-DQT_USE_FAST_CONCATENATION -DQT_USE_FAST_OPERATOR_PLUS)
add_definitions(
    ${QT_DEFINITIONS}
    ${QT_QTDBUS_DEFINITIONS}
    ${KDE4_DEFINITIONS}
    )

include_directories(
    ${QDBUS_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}
    ${KDE4_INCLUDES}
    ${EXIV2_INCLUDE_DIR}
    ${KDCRAW_INCLUDE_DIR}
    )

## dirs to build
add_subdirectory(lib)
add_subdirectory(app)
add_subdirectory(importer)
add_subdirectory(part)
macro_optional_add_subdirectory(tests)
add_subdirectory(icons)
add_subdirectory(images)
add_subdirectory(cursors)
add_subdirectory(color-schemes)
macro_optional_add_subdirectory(doc)

configure_file(config-gwenview.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-gwenview.h)

macro_display_feature_log()
