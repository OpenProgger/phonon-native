cmake_minimum_required(VERSION 3.28 FATAL_ERROR)
project(PhononNative VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_FLAGS
    "-O2 -Weverything -Wno-pre-c++17-compat -Wno-c++98-compat -Wno-switch-default -Wno-weak-vtables -Wno-unsafe-buffer-usage"
)

include(FeatureSummary)
add_definitions(-DPHONON_MPV_VERSION="${PROJECT_VERSION}")

find_package(ECM 6.0 NO_MODULE)
set(KDE_COMPILERSETTINGS_LEVEL "6.0")
set_package_properties(
  ECM PROPERTIES
  TYPE REQUIRED
  DESCRIPTION "Extra CMake Modules"
  URL "https://api.kde.org/frameworks/extra-cmake-modules/html/index.html")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

include(KDEInstallDirs6)
include(KDECMakeSettings)
include(ECMSetupVersion)

find_package(Qt6 REQUIRED COMPONENTS Core Quick Multimedia)

find_package(Phonon4Qt6 4.12.0 NO_MODULE)
set_package_properties(
  Phonon4Qt6 PROPERTIES
  TYPE REQUIRED
  DESCRIPTION "Phonon core library"
  URL "https://api.kde.org/phonon/html/index.html")

find_package(Phonon4Qt6Experimental 4.12.0 NO_MODULE)
set_package_properties(
  Phonon4Qt6Experimental PROPERTIES
  TYPE OPTIONAL
  DESCRIPTION "Phonon experimental library"
  URL "https://api.kde.org/phonon/html/index.html")
if(Phonon4Qt6Experimental_FOUND)
  set(PHONON_EXPERIMENTAL TRUE)
endif()

ecm_setup_version(PROJECT VARIABLE_PREFIX PHONON_NATIVE)
add_subdirectory(src)

feature_summary(FATAL_ON_MISSING_REQUIRED_PACKAGES WHAT ALL)
