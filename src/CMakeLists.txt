add_executable ("ayatana-appindicator-display" main.c indicator.c elevation.c)
target_compile_definitions ("ayatana-appindicator-display" PRIVATE G_LOG_DOMAIN="ayatana-appindicator-display" LOCALEDIR="${CMAKE_INSTALL_FULL_LOCALEDIR}" GETTEXT_PACKAGE="ayatana-appindicator-display")
target_link_libraries ("ayatana-appindicator-display" PRIVATE PkgConfig::PROJECT -lm)
install (TARGETS "ayatana-appindicator-display" RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
