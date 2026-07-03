add_compile_flags(LD
        -Wl,--reread_libs -Wl,--diag_suppress=10063 -Wl,--diag_wrap=off -Wl,--display_error_number
        -Wl,--warn_sections  -Wl,--xml_link_info="linkInfo.xml" -Wl,--rom_model
)
