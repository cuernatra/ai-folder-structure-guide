cmake_minimum_required(VERSION 3.16)

project(ai_folder_structure_guide VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Quick Network)

qt_standard_project_setup(REQUIRES 6.9)

qt_add_executable(appai_folder_structure_guide
    main.cpp
    FolderTreeService.cpp
    FolderTreeService.h
    OllamaService.cpp
    OllamaService.h
)

qt_add_qml_module(appai_folder_structure_guide
    URI ai_folder_structure_guide
    QML_FILES
        Main.qml
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appai_folder_structure_guide PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appai_folder_structure_guide
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appai_folder_structure_guide
    PRIVATE Qt6::Quick Qt6::Network
)

if(WIN32)
    get_target_property(_qt_qmake_executable Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qt_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET appai_folder_structure_guide POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                --qmldir "${CMAKE_SOURCE_DIR}"
                "$<TARGET_FILE:appai_folder_structure_guide>"
            COMMENT "Deploying Qt runtime and QML dependencies with windeployqt"
            VERBATIM
        )
    else()
        message(WARNING "windeployqt not found. Qt runtime files will not be copied automatically.")
    endif()
endif()

install(TARGETS appai_folder_structure_guide
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
