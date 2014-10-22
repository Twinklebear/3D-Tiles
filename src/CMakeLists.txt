add_executable(3DTiles main.cpp util.cpp gl_core_4_4.c)
target_link_libraries(3DTiles ${SDL2_LIBRARY} ${OPENGL_LIBRARIES})

install(TARGETS 3DTiles DESTINATION ${3DTiles_INSTALL_DIR})

