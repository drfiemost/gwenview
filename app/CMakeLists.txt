include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${EXIV2_INCLUDE_DIR}
    ${LIBKONQ_INCLUDE_DIR}
    )

if (KIPI_FOUND)
    include_directories(${KIPI_INCLUDE_DIR})
endif()

# For lib/gwenviewconfig.h and config-gwenview.h
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}/..
    )

set(gwenview_SRCS
    abstractcontextmanageritem.cpp
    configdialog.cpp
    gvcore.cpp
    documentinfoprovider.cpp
    viewmainpage.cpp
    fileoperations.cpp
    filtercontroller.cpp
    folderviewcontextmanageritem.cpp
    fullscreencontent.cpp
    infocontextmanageritem.cpp
    imagemetainfodialog.cpp
    imageopscontextmanageritem.cpp
    fileopscontextmanageritem.cpp
    main.cpp
    mainwindow.cpp
    preloader.cpp
    saveallhelper.cpp
    savebar.cpp
    sidebar.cpp
    startmainpage.cpp
    thumbnailviewhelper.cpp
    browsemainpage.cpp
    )

if (KIPI_FOUND)
    set (gwenview_SRCS
        ${gwenview_SRCS}
        kipiexportaction.cpp
        kipiimagecollectionselector.cpp
        kipiinterface.cpp
        kipiuploadwidget.cpp
        )
endif()

kde4_add_ui_files(gwenview_SRCS
    advancedconfigpage.ui
    fullscreenconfigwidget.ui
    generalconfigpage.ui
    imageviewconfigpage.ui
    semanticinfodialog.ui
    startmainpage.ui
    browsemainpage.ui
    )

kde4_add_app_icon(gwenview_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/../icons/hi*-apps-gwenview.png")

kde4_add_executable(gwenview ${gwenview_SRCS})

target_link_libraries(gwenview
    ${KDE4_KFILE_LIBS}
    gwenviewlib
    ${KDE4_KIO_LIBS}
    ${LIBKONQ_LIBRARY}
    ${QT_QTCORE_LIBRARY}
    )

target_link_libraries(gwenview LINK_INTERFACE_LIBRARIES ${KDE4_KIO_LIBS} ${QT_QTCORE_LIBRARY})

if (KIPI_FOUND)
    target_link_libraries(gwenview ${KIPI_LIBRARIES})
endif()

install(TARGETS gwenview
    ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES gwenviewui.rc
    DESTINATION ${DATA_INSTALL_DIR}/gwenview)

install(PROGRAMS gwenview.desktop
    DESTINATION ${XDG_APPS_INSTALL_DIR})
install(FILES slideshow.desktop
    DESTINATION ${SERVICES_INSTALL_DIR}/ServiceMenus)
