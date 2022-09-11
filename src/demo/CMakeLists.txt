cmake_minimum_required(VERSION 3.0.0)

project(DigitalConductorDemo)

cmake_policy(SET CMP0071 NEW)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.2 COMPONENTS Quick REQUIRED)

include_directories(
  controllers
  components
  gadgets
  qml
  )

set(CPP_SOURCES
  Main.cpp

  controllers/PeerController/PeerController.cpp
  controllers/DeviceController/DeviceController.cpp
  controllers/ReceiverController/ReceiverController.cpp
  controllers/SenderController/SenderController.cpp
  controllers/LicenseController/LicenseController.cpp

  gadgets/PeerInfo/PeerInfo.cpp
  gadgets/AudioDeviceInfo/AudioDeviceInfo.cpp
  gadgets/AudioDirection/AudioDirection.cpp

  qml/QmlEngine/QmlEngine.cpp
  )

set(QML_SOURCES
  common/common.js

  Main.qml
  Main.js

  components/DCRoundSwitch/DCRoundSwitch.qml
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
)

qt_add_qml_module(DigitalConductorDemo
  URI digitalconductor
  VERSION 1.0
  QML_FILES ${QML_SOURCES}
  )

add_dependencies(DigitalConductorDemo
  DigitalConductorDeps
  DigitalConductorLib
  )

target_link_libraries(DigitalConductorDemo
  PRIVATE DigitalConductorLib
  )

target_link_libraries(DigitalConductorDemo
  PRIVATE Qt6::Quick
  )

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
