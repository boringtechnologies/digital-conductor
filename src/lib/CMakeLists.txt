cmake_minimum_required(VERSION 3.0.0)

project(DigitalConductorLib)

add_library(DigitalConductorLib STATIC
  "auto-discovery/DiscoveryManager.cpp"
  "licensing/LicenseManager.cpp"
  "licensing/AES.cpp"
  )

add_dependencies(DigitalConductorLib
  DigitalConductorDeps
  )

set_target_properties(DigitalConductorLib PROPERTIES
  OUTPUT_NAME "digitalconductor"
  )
