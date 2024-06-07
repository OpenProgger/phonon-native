add_definitions(-DPHONON_BACKEND_VERSION_4_10)

add_library(phonon_native_qt6 MODULE)

target_sources(
  phonon_native_qt6
  PRIVATE FILE_SET
          CXX_MODULES
          FILES
          backend.cxx
          mediaobject.cxx
          audiooutput.cxx
          audiodataoutput.cxx
          videowidget.cxx
          volumefadereffect.cxx
          sinknode.cxx)

target_sources(phonon_native_qt6 PRIVATE video.qrc)

# if(PHONON_EXPERIMENTAL) target_sources(phonon_native_qt6 PRIVATE ) endif()

target_link_libraries(phonon_native_qt6 Phonon::phonon4qt6 Qt6::Core Qt6::Quick
                      Qt6::Multimedia)
if(PHONON_EXPERIMENTAL)
  target_link_libraries(phonon_native_qt6 Phonon::phonon4qt6experimental)
endif()

install(TARGETS phonon_native_qt6 DESTINATION ${PHONON_BACKEND_DIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/phonon-native.json.in
               ${CMAKE_CURRENT_BINARY_DIR}/phonon-native.json @ONLY)
