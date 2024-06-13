cmake_minimum_required(VERSION 3.16)
project(
  dsbsu
  HOMEPAGE_URL https://github.com/mrclksr/DSBSu
  LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(PROGRAM ${CMAKE_PROJECT_NAME})
set(SCRIPTS_DIR "${CMAKE_INSTALL_PREFIX}/libexec")

if(NOT DATADIR)
  set(DATADIR ${CMAKE_INSTALL_PREFIX}/share/${CMAKE_PROJECT_NAME})
endif(NOT DATADIR)
set(PATH_DOC_URL "https://github.com/mrclksr/DSBSu/blob/master/README.adoc")
set(LOCALE_PATH "${DATADIR}")
set(HELPER_PATH ${SCRIPTS_DIR}/dsbsu-helper)
if(PROJECT_VERSION)
  set(PROGRAM_VERSION ${PROJECT_VERSION})
else()
  execute_process(
    COMMAND sh -c "git describe --abbrev=12 --always --tags | tr -d '\n'"
    OUTPUT_VARIABLE PROGRAM_VERSION)
  set(PROGRAM_VERSION "git hash: ${PROGRAM_VERSION}")
endif()

set(SOURCE_FILES
    src/mainwin.cpp
    src/mainwin.h
    src/main.cpp
    lib/libdsbsu.c
    lib/libdsbsu.h
    lib/qt-helper/qt-helper.cpp
    lib/qt-helper/qt-helper.h)

find_package(Qt6 REQUIRED COMPONENTS Widgets LinguistTools)

qt_standard_project_setup()

set(TS_FILES locale/dsbsu_de.ts locale/dsbsu_fr.ts)

add_executable(dsbsu ${SOURCE_FILES})

qt_add_translations(dsbsu TS_FILES ${TS_FILES} QM_FILES_OUTPUT_VARIABLE
                    QM_FILES)

target_include_directories(
  dsbsu PUBLIC "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}"
               "${CMAKE_CURRENT_SOURCE_DIR}/lib")

configure_file(defs.h.in defs.h @ONLY)

set(LIBS Qt6::Widgets pthread util)

target_link_libraries(dsbsu PRIVATE ${LIBS})

add_custom_command(
  OUTPUT dsbsudo
  COMMAND
    sh -c
    "sed -E 's#@INSTALLDIR@#${SCRIPTS_DIR}#g' ${CMAKE_CURRENT_SOURCE_DIR}/dsbsudo.in >dsbsudo"
)
add_custom_target(build_dsbsudo ALL DEPENDS "dsbsudo")

install(DIRECTORY COMPONENT DESTINATION "man/man1")
install(TARGETS dsbsu DESTINATION bin)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/dsbsudo DESTINATION bin)
install(PROGRAMS dsbsudo-askpass DESTINATION libexec)
install(PROGRAMS dsbsu-helper DESTINATION libexec)
install(FILES "${CMAKE_PROJECT_NAME}.desktop" DESTINATION share/applications)
install(FILES ${QM_FILES} DESTINATION share/${CMAKE_PROJECT_NAME})
install(FILES man/dsbsu.1 DESTINATION man/man1)
install(FILES man/dsbsudo.1 DESTINATION man/man1)
