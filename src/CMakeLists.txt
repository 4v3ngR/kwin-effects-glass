add_subdirectory(kcm)

# Read compat preambles
file(READ shaders/compat_core.glsl COMPAT_CORE)
file(READ shaders/compat_legacy.glsl COMPAT_LEGACY)
file(READ shaders/glass.glsl GLASS_SHADER)
file(READ shaders/noise.glsl NOISE_SHADER)

# Generate a core and legacy shader variant from a single .glsl source.
# Output goes into generated/ to keep source shaders/ clean.
function(generate_shader_variants input_glsl output_base)
    file(READ "${input_glsl}" SHADER_SRC)

    # Expand includes
    string(REPLACE "#include \"glass.glsl\"" "${GLASS_SHADER}" SHADER_SRC_EXPANDED "${SHADER_SRC}")
    string(REPLACE "#include \"noise.glsl\"" "${NOISE_SHADER}" SHADER_SRC_EXPANDED "${SHADER_SRC_EXPANDED}")

    # Core variant
    file(WRITE "generated/${output_base}_core${ARGN}" "${COMPAT_CORE}\n${SHADER_SRC_EXPANDED}")

    # Legacy variant
    file(WRITE "generated/${output_base}${ARGN}" "${COMPAT_LEGACY}\n${SHADER_SRC_EXPANDED}")
endfunction()

# Generate vertex shaders
generate_shader_variants(shaders/vertex.glsl vertex .vert)
generate_shader_variants(shaders/onscreen_rounded_vertex.glsl onscreen_rounded .vert)

# Generate fragment shaders
generate_shader_variants(shaders/downsample.glsl downsample .frag)
generate_shader_variants(shaders/upsample.glsl upsample .frag)
generate_shader_variants(shaders/onscreen_rounded.glsl onscreen_rounded .frag)

set(glass_SOURCES
    blur.cpp
    settings.cpp
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
