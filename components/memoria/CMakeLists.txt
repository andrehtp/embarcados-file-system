if(IDF_TARGET STREQUAL "linux")
    set(srcs "memoria_linux.c")
    set(priv_reqs "")
else()
    set(srcs "memoria_esp32.c")
    set(priv_reqs driver)
endif()

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "include"
    PRIV_REQUIRES ${priv_reqs}
)
