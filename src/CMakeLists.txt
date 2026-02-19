add_subdirectory(kcm)

function(replace_shader_include input output)
    file(READ shaders/roundedcorners.glsl ROUNDEDCORNERS_SHADER)
    file(READ "${input}" SHADER)
    string(REPLACE "#include \"roundedcorners.glsl\"" "${ROUNDEDCORNERS_SHADER}" SHADER "${SHADER}")
    file(WRITE "${output}" "${SHADER}")
endfunction()

replace_shader_include(shaders/texture.glsl shaders/texture.frag)
replace_shader_include(shaders/texture_core.glsl shaders/texture_core.frag)
replace_shader_include(shaders/upsample.glsl shaders/upsample.frag)
replace_shader_include(shaders/upsample_core.glsl shaders/upsample_core.frag)

set(glass_SOURCES
    blur.cpp
    blur.qrc
    main.cpp
    settings.cpp
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
