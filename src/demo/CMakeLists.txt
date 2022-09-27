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
find_package(PkgConfig REQUIRED)

pkg_check_modules(GST REQUIRED
        gobject-2.0
        glib-2.0
        gstreamer-1.0
        gstreamer-app-1.0
        gstreamer-net-1.0)

include_directories(
    ../core
    controllers
    components
    gadgets
    qml)

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}/../core")

set(CPP_SOURCES
  Main.cpp
  registrator.cpp

  controllers/appcontroller.cpp
  controllers/appcontroller.h
  controllers/PeerController/PeerController.cpp
  controllers/AudioDeviceController/AudioDeviceController.cpp
  controllers/VideoDeviceController/VideoDeviceController.cpp
  controllers/ReceiverController/ReceiverController.cpp
  controllers/SenderController/SenderController.cpp
  controllers/LicenseController/LicenseController.cpp

  gadgets/PeerInfo/PeerInfo.cpp
  gadgets/DeviceInfo/DeviceInfo.cpp
  gadgets/DeviceDirection/DeviceDirection.cpp

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
  DigitalConductorDeps
  DigitalConductorLib)

message(${ZLIB_LIBRARIES})

target_link_libraries(DigitalConductorDemo PRIVATE
    digitalconductorcore
    DigitalConductorLib
    DigitalConductorCore_proto
    DigitalConductorCore_lib
    ${ZLIB_LIBRARIES})

target_link_libraries(DigitalConductorDemo PRIVATE
    Qt6::Quick)

set_target_properties(DigitalConductorDemo PROPERTIES
  OUTPUT_NAME "digitalconductor-demo"
  MACOSX_BUNDLE TRUE
  WIN32_EXECUTABLE TRUE
  )

if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
  add_custom_command(
    COMMENT "Stipping DigitalConductorDemo"
    TARGET DigitalConductorDemo
    POST_BUILD
    COMMAND strip "$<TARGET_FILE:DigitalConductorDemo>"
    )
endif()
