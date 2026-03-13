add_subdirectory(kcm)

function(replace_shader_include input output)
    file(READ shaders/glass.glsl GLASS_SHADER)
    file(READ "${input}" SHADER)
    string(REPLACE "#include \"glass.glsl\"" "${GLASS_SHADER}" SHADER "${SHADER}")
    file(WRITE "${output}" "${SHADER}")
endfunction()

# replace_shader_include(shaders/onscreen.glsl shaders/onscreen.frag)
# replace_shader_include(shaders/onscreen_core.glsl shaders/onscreen_core.frag)
replace_shader_include(shaders/onscreen_rounded.glsl shaders/onscreen_rounded.frag)
replace_shader_include(shaders/onscreen_rounded_core.glsl shaders/onscreen_rounded_core.frag)

set(glass_SOURCES
    blur.cpp
    blur.qrc
    main.cpp
)

kconfig_add_kcfg_files(glass_SOURCES
    blurconfig.kcfgc
)

if(GLASS_WAYLAND)
    add_library(glass MODULE ${glass_SOURCES})
    target_link_libraries(glass PRIVATE
        KDecoration3::KDecoration
        KF6::ConfigGui
        KWin::kwin
    )
    install(TARGETS glass DESTINATION ${KDE_INSTALL_PLUGINDIR}/kwin/effects/plugins)
endif()
if(GLASS_X11)
    add_library(glass_x11 MODULE ${glass_SOURCES})
    target_link_libraries(glass_x11 PRIVATE
        KDecoration3::KDecoration
        KF6::ConfigGui
        KWinX11::kwin
        XCB::XCB
    )
    target_compile_definitions(glass_x11 PRIVATE GLASS_X11)
    install(TARGETS glass_x11 DESTINATION ${KDE_INSTALL_PLUGINDIR}/kwin-x11/effects/plugins)
endif()
