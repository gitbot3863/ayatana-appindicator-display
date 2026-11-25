# LICENSE

install (FILES "${CMAKE_SOURCE_DIR}/LICENSE" DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/licenses/ayatana-appindicator-display")

# org.ayatana.appindicator.display.gschema.xml

find_package (GSettings REQUIRED)
add_schema ("org.ayatana.appindicator.display.gschema.xml")

# ayatana-appindicator-display.desktop

find_package (Intltool REQUIRED)
configure_file ("${CMAKE_CURRENT_SOURCE_DIR}/ayatana-appindicator-display.desktop.in" "${CMAKE_CURRENT_BINARY_DIR}/ayatana-appindicator-display.desktop.in" @ONLY)
intltool_merge_translations ("${CMAKE_PROJECT_NAME}.desktop.in" "${CMAKE_PROJECT_NAME}.desktop" ALL UTF8)
install (FILES "${CMAKE_CURRENT_BINARY_DIR}/ayatana-appindicator-display.desktop" DESTINATION "${CMAKE_INSTALL_FULL_SYSCONFDIR}/xdg/autostart")

# icons

install (DIRECTORY "icons/" DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/icons/hicolor/scalable/status")
