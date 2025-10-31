cmake_minimum_required(VERSION 3.16)

project(OSCWidgets LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_SUPPRESS_REGENERATION ON)

if(MSVC)
  add_compile_options(/W4 /MP /wd4996)
  set(ENV{QTDIR} "C:/Qt/6.9.2/msvc2022_64")
elseif(XCODE)
  set(CMAKE_XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym")
  set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO")
  set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual")
  set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
  set(CMAKE_OSX_DEPLOYMENT_TARGET "13.3")
  set(ENV{QTDIR} "$ENV{HOME}/Qt/6.9.2/macos")
endif()

set(CMAKE_PREFIX_PATH "$ENV{QTDIR}")
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network Qml)
qt_standard_project_setup()

include_directories(
  "../EosSyncLib/EosSyncLib"
  "OSCWidgets"
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

file(GLOB HEADERS
  "OSCWidgets/*.h"
)

file(GLOB SOURCES
  "OSCWidgets/*.cpp"
)

set(EOS_SYNC_LIBS_SOURCES
  "../EosSyncLib/EosSyncLib/EosLog.cpp"
  "../EosSyncLib/EosSyncLib/EosTcp.cpp"
  "../EosSyncLib/EosSyncLib/EosTimer.cpp"
  "../EosSyncLib/EosSyncLib/EosUdp.cpp"
  "../EosSyncLib/EosSyncLib/OSCParser.cpp"
)

if(WIN32)
  set(EOS_SYNC_LIBS_SOURCES ${EOS_SYNC_LIBS_SOURCES}
    "../EosSyncLib/EosSyncLib/EosTcp_Win.cpp"
    "../EosSyncLib/EosSyncLib/EosUdp_Win.cpp"
  )

  set(RESOURCE_FILES
    "OSCWidgets/OSCWidgets.rc"
    "OSCWidgets/icon1.ico"
    "OSCWidgets/qt.conf"
    "OSCWidgets/qt.qrc"
  )
elseif(APPLE)
  include_directories(
    "OSCWidgets/Mac"
  )

  set(EOS_SYNC_LIBS_SOURCES ${EOS_SYNC_LIBS_SOURCES}
    "../EosSyncLib/EosSyncLib/EosTcp_Mac.cpp"
    "../EosSyncLib/EosSyncLib/EosUdp_Mac.cpp"
  )

  file(GLOB MAC_HEADERS
    "OSCWidgets/Mac/*.h"
  )

  set(HEADERS ${HEADERS} ${MAC_HEADERS})

  file(GLOB MAC_SOURCES
    "OSCWidgets/Mac/*.cpp"
    "OSCWidgets/Mac/*.mm"
  )

  set(SOURCES ${SOURCES} ${MAC_SOURCES})

  set(RESOURCE_FILES
    "OSCWidgets/qt.conf"
    "OSCWidgets/qt.qrc"
  )

  file(GLOB BUNDLE_RESOURCE_FILES
    "icon.icns"
  )

  set_source_files_properties(${BUNDLE_RESOURCE_FILES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  source_group("Resources" FILES ${BUNDLE_RESOURCE_FILES})
endif()

add_compile_definitions(NOMINMAX)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${HEADERS} ${SOURCES})
source_group("Resource Files" FILES ${RESOURCE_FILES})
qt_add_executable(${PROJECT_NAME} ${SOURCES} ${EOS_SYNC_LIBS_SOURCES} ${HEADERS} ${RESOURCE_FILES} ${BUNDLE_RESOURCE_FILES})
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets Qt6::Gui Qt6::Network Qt6::Qml)

if(WIN32)
  target_link_libraries(${PROJECT_NAME} PRIVATE winmm iphlpapi)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
  WIN32_EXECUTABLE ON
  MACOSX_BUNDLE ON
  MACOSX_BUNDLE_GUI_IDENTIFIER org.etc.${PROJECT_NAME}
  MACOSX_BUNDLE_ICON_FILE "icon.icns"
)

if(WIN32)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND cmd /C \"\"$ENV{QTDIR}\\bin\\windeployqt\" --force --no-translations --no-opengl-sw --no-compiler-runtime \"$<TARGET_FILE:${PROJECT_NAME}>\"\"
  )
elseif(APPLE)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND rm -rf \"$<TARGET_FILE_DIR:${PROJECT_NAME}>/../../../${PROJECT_NAME}.dmg\"    
    COMMAND "$ENV{QTDIR}/bin/macdeployqt" \"$<TARGET_FILE_DIR:${PROJECT_NAME}>/../..\" -dmg
  )
endif()
