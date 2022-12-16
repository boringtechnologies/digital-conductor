cmake_minimum_required(VERSION 3.0.0)

project(DigitalConductorDemo)

cmake_policy(SET CMP0071 NEW)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(
  Qt6 6.2
  COMPONENTS Quick
  REQUIRED)

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
    qml/QmlEngine/QmlEngine.cpp)

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
    components/DCSlider/DCSlider.qml
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
    views/LicenseView/LicenseView.qml)

add_custom_command(OUTPUT "${PROJECT_SOURCE_DIR}/qmlindex.qrc"
  DEPENDS "${PROJECT_SOURCE_DIR}/qmlindex.py"
  COMMAND "${PYTHON}" "${PROJECT_SOURCE_DIR}/qmlindex.py" "${PROJECT_SOURCE_DIR}/qmlindex.qrc" "/dc" ${QML_SOURCES}
  COMMENT "Generating qml index")

set(QRC_SOURCES "${PROJECT_SOURCE_DIR}/qmlindex.qrc" "${PROJECT_SOURCE_DIR}/images/_images.qrc")

qt_add_executable(DigitalConductorDemo ${CPP_SOURCES} ${QRC_SOURCES})

qt_add_qml_module(
  DigitalConductorDemo
  URI
  digitalconductor
  VERSION
  1.0
  QML_FILES
  ${QML_SOURCES})

if(CMAKE_BUILD_TYPE MATCHES Debug)
  message("-- Qml debugging enabled")
  target_compile_definitions(DigitalConductorDemo PRIVATE QT_DECLARATIVE_DEBUG QT_QML_DEBUG)
endif()

add_dependencies(DigitalConductorDemo DigitalConductorCore_lib DigitalConductorLib)

target_link_libraries(
  DigitalConductorDemo
  PRIVATE DigitalConductorLib
          DigitalConductorCore_proto
          DigitalConductorCore_lib
          fmt::fmt)

target_include_directories(DigitalConductorDemo PRIVATE ${LibJuice_INCLUDE_DIRS})

target_link_libraries(DigitalConductorDemo PRIVATE Qt6::Quick)

set_target_properties(
  DigitalConductorDemo
  PROPERTIES OUTPUT_NAME "digitalconductor-demo"
             MACOSX_BUNDLE TRUE
             WIN32_EXECUTABLE TRUE
             MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)

if(${CMAKE_BUILD_TYPE} STREQUAL "Release")
  add_custom_command(TARGET DigitalConductorDemo POST_BUILD
    COMMAND strip "$<TARGET_FILE:DigitalConductorDemo>"
    COMMENT "Stipping DigitalConductorDemo")
endif()
