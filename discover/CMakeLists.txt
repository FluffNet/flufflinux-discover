add_subdirectory(icons)

include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/..)

ecm_qt_declare_logging_category(plasma_discover_SRCS HEADER discover_debug.h IDENTIFIER DISCOVER_LOG CATEGORY_NAME org.kde.plasma.discover DESCRIPTION "discover" EXPORT DISCOVER)
kconfig_add_kcfg_files(plasma_discover_SRCS discoversettings.kcfgc GENERATE_MOC)

# Avoid problems with sign conversion by warning about them. Selectively in this file because we know there's been
# problems in the past. Ideally we'd move towards applying this everywhere.
# ES.100: Don’t mix signed and unsigned arithmetic
# https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#res-mix
# https://bugs.kde.org/show_bug.cgi?id=513676
set_source_files_properties(DiscoverObject.cpp PROPERTIES COMPILE_OPTIONS "-Wsign-conversion")

add_executable(plasma-discover ${plasma_discover_SRCS}
    main.cpp
    DiscoverObject.cpp
    DiscoverDeclarativePlugin.cpp

    AbstractAppsModel.cpp
    OdrsAppsModel.cpp
    FeaturedModel.cpp
    LimitedRowCountProxyModel.cpp
    UnityLauncher.cpp
    ReadFile.cpp
    PowerManagementInterface.cpp
    AppUpdateSettings.cpp

    DiscoverObject.h
    DiscoverDeclarativePlugin.h
    AppUpdateSettings.h

    FeaturedModel.h
    UnityLauncher.h
    ReadFile.h


    resources.qrc
    RefreshNotifier.cpp
)
add_executable(Plasma::Discover ALIAS plasma-discover)
set_target_properties(plasma-discover PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR}/plasma-discover)

target_link_libraries(plasma-discover PUBLIC
                                     KF6::Crash
                                     KF6::CoreAddons
                                     KF6::DBusAddons
                                     KF6::I18n
                                     KF6::ConfigGui
                                     KF6::KIOCore
                                     KF6::KIOGui
                                     KF6::WindowSystem
                                     KF6::Notifications
                                     KF6::JobWidgets
                                     KF6::StatusNotifierItem
                                     KF6::I18nQml
                                     KirigamiApp
                                     Qt::Widgets
                                     Qt::Quick
                                     Qt::QuickControls2
                                     Discover::Common
)

if (TARGET Qt::WebView)
    target_link_libraries(plasma-discover PUBLIC Qt::WebView)
    target_compile_definitions(plasma-discover PUBLIC -DWITH_QTWEBVIEW=1)
else()
    target_compile_definitions(plasma-discover PUBLIC -DWITH_QTWEBVIEW=0)
endif()

install(TARGETS plasma-discover ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

# Standard desktop file accepts Flatpak files as input.
set(DesktopNoDisplay "false")
set(DesktopMimeType "")
if(Flatpak_FOUND)
  set(DesktopMimeType "${DesktopMimeType}application/vnd.flatpak;application/vnd.flatpak.repo;application/vnd.flatpak.ref;")
endif()
set(DesktopExec "plasma-discover %F")
configure_file(org.kde.discover.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.desktop)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.desktop DESTINATION ${KDE_INSTALL_APPDIR} )

# Support appstream:// URI
set(DesktopNoDisplay "true")
set(DesktopMimeType "x-scheme-handler/appstream;")
set(DesktopExec "plasma-discover %U")
configure_file(org.kde.discover.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.urlhandler.desktop)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.urlhandler.desktop DESTINATION ${KDE_INSTALL_APPDIR} )

if(Flatpak_FOUND)
    # support Flatpak flatpak+https URI
    set(DesktopNoDisplay "true")
    set(DesktopMimeType "x-scheme-handler/flatpak+https;")
    set(DesktopExec "plasma-discover %U")
    configure_file(org.kde.discover.desktop.cmake ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.flatpak.desktop)
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/org.kde.discover.flatpak.desktop DESTINATION ${KDE_INSTALL_APPDIR} )
endif()

install(FILES plasmadiscoverui.rc DESTINATION ${KDE_INSTALL_KXMLGUIDIR}/plasmadiscover)
install(FILES org.kde.discover.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )
