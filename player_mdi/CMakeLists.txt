# Source files
set(PLAYER_MDI_SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/videoplayer.cpp 
    src/decoder_thread.cpp
)

# Header files (for MOC)
set(PLAYER_MDI_HEADERS
    include/mainwindow.h
    include/videoplayer.h 
    include/decoder_thread.h
)

# Create executable
add_executable(player_mdi
    ${PLAYER_MDI_SOURCES}
    ${PLAYER_MDI_HEADERS}
)

# Link Qt libraries
if(Qt6_FOUND)
    target_link_libraries(player_mdi
        PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets
        PRIVATE avcdec
        PRIVATE jmdec
    )
else()
    target_link_libraries(player_mdi
        PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets
        PRIVATE avcdec
        PRIVATE jmdec
    )
endif()

# Include directories
target_include_directories(player_mdi
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../avcdec/include
)

# Platform-specific settings
if(APPLE)
    # macOS specific settings
    set_target_properties(player_mdi PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.example.player-mdi"
        MACOSX_BUNDLE_BUNDLE_NAME "Player MDI"
        MACOSX_BUNDLE_INFO_STRING "H.264 Video Player MDI"
        MACOSX_BUNDLE_LONG_VERSION_STRING "1.0.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
    )
elseif(WIN32)
    # Windows specific settings
    set_target_properties(player_mdi PROPERTIES
        WIN32_EXECUTABLE ON
    )
endif()