cmake_minimum_required(VERSION 3.16)

project(sunrise VERSION 1.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network PrintSupport Sql Concurrent)
if(WIN32)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS AxContainer)
endif()
if(UNIX AND NOT APPLE)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS WebEngine WebEngineWidgets WebChannel)
endif()

set(PROJECT_SOURCES
        main.cpp
        src/approotpaths.h
        src/approotpaths.cpp
        src/sunrisewindow.cpp
        src/sunrisewindow.h
        src/rtfconverter.h
        src/rtfconverter.cpp
        src/usagejournal.h
        src/usagejournal.cpp
        src/summarypanel.h
        src/summarypanel.cpp
        src/clinicalbrowsercontroller.h
        src/clinicalbrowsercontroller.cpp
        src/models.h
        src/apiclient.h
        src/apiclient.cpp
        src/repository.h
        src/repository.cpp
        src/localdatabase.h
        src/localdatabase.cpp
        src/fieldcrypto.h
        src/fieldcrypto.cpp
        src/licenseservice.h
        src/licenseservice.cpp
        src/singleinstance.h
        src/singleinstance.cpp
        src/licenseactivationdialog.h
        src/licenseactivationdialog.cpp
        src/custommessagebox.h
        src/custommessagebox.cpp
        src/imagebutton.h
        src/imagebutton.cpp
        src/appsettings.h
        src/appsettings.cpp
        ui.qrc
)

if(UNIX AND NOT APPLE AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
    list(APPEND PROJECT_SOURCES
        src/smbiosreader.h
        src/smbiosreader.cpp
    )
endif()

if(WIN32)
    list(APPEND PROJECT_SOURCES app_icon.rc)
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(sunrise
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(sunrise SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(sunrise
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(sunrise PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::PrintSupport
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Concurrent
)
if(WIN32)
    target_link_libraries(sunrise PRIVATE Qt${QT_VERSION_MAJOR}::AxContainer)
endif()
if(UNIX AND NOT APPLE)
    target_link_libraries(sunrise PRIVATE
        Qt${QT_VERSION_MAJOR}::WebEngine
        Qt${QT_VERSION_MAJOR}::WebEngineWidgets
        Qt${QT_VERSION_MAJOR}::WebChannel)
endif()

if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.dokitlab.sunrise)
endif()
set_target_properties(sunrise PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME sunrise
)

include(GNUInstallDirs)
install(TARGETS sunrise
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(sunrise)
endif()

if(UNIX AND NOT APPLE)
    add_custom_command(TARGET sunrise POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:sunrise>/assets"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/source.txt"
            "$<TARGET_FILE_DIR:sunrise>/source.txt"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/updates.txt"
            "$<TARGET_FILE_DIR:sunrise>/updates.txt"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/aJournal.rtf"
            "$<TARGET_FILE_DIR:sunrise>/aJournal.rtf"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:sunrise>/data"
            "$<TARGET_FILE_DIR:sunrise>/data/scans"
            "$<TARGET_FILE_DIR:sunrise>/key"
        COMMENT "Copying sunrise assets and creating data/key next to executable"
    )
endif()

if(WIN32)
    find_path(SUNRISE_OPENSSL_DLL_DIR
        NAMES libcrypto-1_1-x64.dll libssl-1_1-x64.dll
        PATHS
            "${Qt5_DIR}/../../../Tools/mingw1120_64/opt/bin"
            "${Qt5_DIR}/../../../Tools/mingw1310_64/opt/bin"
            "${Qt5_DIR}/../../../Tools/mingw810_64/opt/bin"
            "${CMAKE_PREFIX_PATH}/../../Tools/mingw1120_64/opt/bin"
            "${CMAKE_PREFIX_PATH}/../../Tools/mingw1310_64/opt/bin"
            "D:/Qt/Tools/mingw1120_64/opt/bin"
            "D:/Qt/Tools/mingw1310_64/opt/bin"
            "D:/Qt/Tools/mingw810_64/opt/bin"
            "C:/Qt/Tools/mingw1120_64/opt/bin"
            "C:/Qt/Tools/mingw1310_64/opt/bin"
        NO_DEFAULT_PATH
    )
    if(SUNRISE_OPENSSL_DLL_DIR)
        add_custom_command(TARGET sunrise POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${SUNRISE_OPENSSL_DLL_DIR}/libcrypto-1_1-x64.dll"
                "${SUNRISE_OPENSSL_DLL_DIR}/libssl-1_1-x64.dll"
                $<TARGET_FILE_DIR:sunrise>
            COMMENT "Copying OpenSSL DLLs for HTTPS"
        )
    else()
        message(WARNING "OpenSSL 1.1 DLLs not found. HTTPS to dokitlab.ru will fail until libcrypto/libssl are copied next to sunrise.exe")
    endif()

    add_custom_command(TARGET sunrise POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:sunrise>/assets"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/source.txt"
            "$<TARGET_FILE_DIR:sunrise>/source.txt"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/updates.txt"
            "$<TARGET_FILE_DIR:sunrise>/updates.txt"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/aJournal.rtf"
            "$<TARGET_FILE_DIR:sunrise>/aJournal.rtf"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:sunrise>/data"
            "$<TARGET_FILE_DIR:sunrise>/data/scans"
            "$<TARGET_FILE_DIR:sunrise>/key"
        COMMENT "Copying assets/source and creating data/key next to executable"
    )
endif()
