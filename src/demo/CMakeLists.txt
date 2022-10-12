cmake_minimum_required(VERSION 3.0.0)

project(DigitalConductorDemo)

cmake_policy(SET CMP0071 NEW)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.2 COMPONENTS Quick REQUIRED)

set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
find_package(ZLIB REQUIRED)

find_package(LibJuice REQUIRED)
find_package(Threads REQUIRED)

if(WITH_CONAN)
    find_package(gRPC REQUIRED)
    find_package(glib REQUIRED)
    find_package(gstreamer REQUIRED)
    find_package(gst-plugins-base REQUIRED)
    find_package(gst-plugins-good REQUIRED)
    find_package(gst-plugins-bad REQUIRED)
    find_package(gst-libav REQUIRED)

    set(GST_INCLUDE_DIRS "${glib_INCLUDE_DIRS};${gstreamer_INCLUDE_DIRS};${gst-plugins-base_INCLUDE_DIRS};${gst-plugins-good_INCLUDE_DIRS};${gst-plugins-bad_INCLUDE_DIRS};${gst-libav_INCLUDE_DIRS}")
    set(GST_LIBRARY_DIRS "")
    set(GST_LIBRARIES "${gstreamer_LIBRARIES};${gst-plugins-base_LIBRARIES};${gst-plugins-good_LIBRARIES};${gst-plugins-bad_LIBRARIES};${gst-libav_LIBRARIES}")

    if(APPLE)
        find_library(io_surface_lib IOSurface)
        list(APPEND GST_LIBRARIES ${io_surface_lib})
    endif()

    set(GST_USE_STATIC_PLUGINS 1)

else(WITH_CONAN)
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(GST REQUIRED
        gobject-2.0
        glib-2.0
        gstreamer-1.0
        gstreamer-app-1.0
        gstreamer-net-1.0
    )

    set(GST_USE_STATIC_PLUGINS 0)

    include_directories(${CMAKE_SOURCE_DIR}/src/core/include
                    ${GST_INCLUDE_DIRS})

    link_directories(${GST_LIBRARY_DIRS})
endif()

include_directories(
    ../core
    controllers
    components
    gadgets
    models
    qml)

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}/../core")

set(CPP_SOURCES
  Main.cpp
  registrator.cpp

  controllers/appcontroller.cpp
  controllers/appcontroller.h
  controllers/PeerController/PeerController.cpp
  controllers/LicenseController/LicenseController.cpp

  gadgets/PeerInfo/PeerInfo.cpp
  gadgets/DeviceInfo/DeviceInfo.cpp
  gadgets/DeviceDirection/DeviceDirection.cpp

  models/peermodel.h
  models/peermodel.cpp
  models/devicemodel.h
  models/devicemodel.cpp

  qml/QmlEngine/QmlEngine.cpp
  )

set(QML_SOURCES
  common/common.js

  Main.qml
  Main.js

  components/DCRoundSwitch/DCRoundSwitch.qml
  components/DCRoundSwitchWithTextField/DCRoundSwitchWithTextField.qml
  components/DCText/DCText.qml
  components/TrayTabRect/TrayTabRect.qml
  components/DCComboBox/DCComboBox.qml
  components/DCTextField/DCTextField.qml
  components/DCLinkButton/DCLinkButton.qml
  components/DCTextArea/DCTextArea.qml
  components/DCButton/DCButton.qml

  views/AppGui/AppGui.qml
  views/AppGui/AppGui.js
  views/PeerListView/PeerList/PeerList.qml
  views/PeerListView/PeerListView.qml
  views/PeerListView/PeerListView.js
  views/ReceiverView/ReceiverView.qml
  views/ReceiverView/ReceiverView.js
  views/SenderView/SenderView.qml
  views/SenderView/SenderView.js
  views/RegisterLicenseView/RegisterLicenseView.qml
  views/RegisterLicenseView/RegisterLicenseView.js

  views/LicenseView/LicenseView.qml
)

add_custom_command(
  COMMENT "Generating qml index"
  OUTPUT
    "${PROJECT_SOURCE_DIR}/qmlindex.qrc"
  DEPENDS
    "${PROJECT_SOURCE_DIR}/qmlindex.py"
  COMMAND
    "${PYTHON}" "${PROJECT_SOURCE_DIR}/qmlindex.py"
      "${PROJECT_SOURCE_DIR}/qmlindex.qrc"
      "/dc"
      ${QML_SOURCES}
  )

qt_add_executable(DigitalConductorDemo
  ${CPP_SOURCES}
  "${PROJECT_SOURCE_DIR}/qmlindex.qrc"
  "${PROJECT_SOURCE_DIR}/images/_images.qrc"
)

qt_add_qml_module(DigitalConductorDemo
  URI digitalconductor
  VERSION 1.0
  QML_FILES ${QML_SOURCES}
  )

add_dependencies(DigitalConductorDemo
  DigitalConductorCore_lib
  DigitalConductorLib)

target_link_libraries(DigitalConductorDemo PRIVATE
    digitalconductorcore
    DigitalConductorLib
    DigitalConductorCore_proto
    DigitalConductorCore_lib)

target_include_directories(DigitalConductorDemo PRIVATE
    ${LibJuice_INCLUDE_DIRS}
    ${gRPC_gRPC_INCLUDE_DIRS}
    ${GST_INCLUDE_DIRS})

target_link_libraries(DigitalConductorDemo PRIVATE
    Qt6::Quick)

set_target_properties(DigitalConductorDemo PROPERTIES
  OUTPUT_NAME "digitalconductor-demo"
  MACOSX_BUNDLE TRUE
  WIN32_EXECUTABLE TRUE
  MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)

if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
  add_custom_command(
    COMMENT "Stipping DigitalConductorDemo"
    TARGET DigitalConductorDemo
    POST_BUILD
    COMMAND strip "$<TARGET_FILE:DigitalConductorDemo>"
    )
endif()
