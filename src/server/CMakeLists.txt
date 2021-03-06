set(SERVER_LIB_SRCS
    buffer_interface.cpp
    clientconnection.cpp
    compositor_interface.cpp
    datadevice_interface.cpp
    datadevicemanager_interface.cpp
    dataoffer_interface.cpp
    datasource_interface.cpp
    display.cpp
    dpms_interface.cpp
    global.cpp
    idle_interface.cpp
    fakeinput_interface.cpp
    keyboard_interface.cpp
    outputconfiguration_interface.cpp
    outputchangeset.cpp
    outputmanagement_interface.cpp
    outputdevice_interface.cpp
    logging.cpp
    output_interface.cpp
    pointer_interface.cpp
    plasmashell_interface.cpp
    plasmawindowmanagement_interface.cpp
    qtsurfaceextension_interface.cpp
    region_interface.cpp
    resource.cpp
    seat_interface.cpp
    slide_interface.cpp
    shadow_interface.cpp
    blur_interface.cpp
    contrast_interface.cpp
    server_decoration_interface.cpp
    shell_interface.cpp
    surface_interface.cpp
    subcompositor_interface.cpp
    touch_interface.cpp
    textinput_interface.cpp
    textinput_interface_v0.cpp
    textinput_interface_v2.cpp
    xdgshell_interface.cpp
    xdgshell_v5_interface.cpp
)

ecm_add_wayland_server_protocol(SERVER_LIB_SRCS
    PROTOCOL ${KWAYLAND_SOURCE_DIR}/src/client/protocols/output-management.xml
    BASENAME output-management
)

ecm_add_wayland_server_protocol(SERVER_LIB_SRCS
    PROTOCOL ${KWAYLAND_SOURCE_DIR}/src/client/protocols/outputdevice.xml
    BASENAME org_kde_kwin_outputdevice
)

ecm_add_wayland_server_protocol(SERVER_LIB_SRCS
    PROTOCOL ${KWAYLAND_SOURCE_DIR}/src/client/protocols/plasma-shell.xml
    BASENAME plasma-shell
)

ecm_add_wayland_server_protocol(SERVER_LIB_SRCS
    PROTOCOL ${KWAYLAND_SOURCE_DIR}/src/client/protocols/plasma-window-management.xml
    BASENAME plasma-window-management
)

ecm_add_wayland_server_protocol(SERVER_LIB_SRCS
    PROTOCOL ${KWAYLAND_SOURCE_DIR}/src/client/protocols/surface-extension.xml
    BASENAME qt-surface-extension
)

ecm_add_wayland_server_protocol(SERVER_LIB_SRCS
    PROTOCOL ${KWAYLAND_SOURCE_DIR}/src/client/protocols/idle.xml
    BASENAME idle
)

ecm_add_wayland_server_protocol(SERVER_LIB_SRCS
    PROTOCOL ${KWAYLAND_SOURCE_DIR}/src/client/protocols/fake-input.xml
    BASENAME fake-input
)

ecm_add_wayland_server_protocol(SERVER_LIB_SRCS
    PROTOCOL ${KWAYLAND_SOURCE_DIR}/src/client/protocols/shadow.xml
    BASENAME shadow
)
ecm_add_wayland_server_protocol(SERVER_LIB_SRCS
    PROTOCOL ${KWAYLAND_SOURCE_DIR}/src/client/protocols/dpms.xml
    BASENAME dpms
)

ecm_add_wayland_server_protocol(SERVER_LIB_SRCS
    PROTOCOL ${KWAYLAND_SOURCE_DIR}/src/client/protocols/blur.xml
    BASENAME blur
)

ecm_add_wayland_server_protocol(SERVER_LIB_SRCS
    PROTOCOL ${KWAYLAND_SOURCE_DIR}/src/client/protocols/contrast.xml
    BASENAME contrast
)

ecm_add_wayland_server_protocol(SERVER_LIB_SRCS
    PROTOCOL ${KWAYLAND_SOURCE_DIR}/src/client/protocols/slide.xml
    BASENAME slide
)

ecm_add_wayland_server_protocol(SERVER_LIB_SRCS
    PROTOCOL ${KWAYLAND_SOURCE_DIR}/src/client/protocols/server-decoration.xml
    BASENAME server_decoration
)

ecm_add_wayland_server_protocol(SERVER_LIB_SRCS
    PROTOCOL ${KWAYLAND_SOURCE_DIR}/src/client/protocols/text-input.xml
    BASENAME text
)

ecm_add_wayland_server_protocol(SERVER_LIB_SRCS
    PROTOCOL ${KWAYLAND_SOURCE_DIR}/src/client/protocols/text-input-unstable-v2.xml
    BASENAME text-input-unstable-v2
)

ecm_add_wayland_server_protocol(SERVER_LIB_SRCS
    PROTOCOL ${KWAYLAND_SOURCE_DIR}/src/client/protocols/xdg-shell-unstable-v5.xml
    BASENAME xdg-shell-v5
)

add_library(KF5WaylandServer ${SERVER_LIB_SRCS})
generate_export_header(KF5WaylandServer
    BASE_NAME
        KWaylandServer
    EXPORT_FILE_NAME
        KWayland/Server/kwaylandserver_export.h
)


add_library(KF5::WaylandServer ALIAS KF5WaylandServer)

target_include_directories(KF5WaylandServer INTERFACE "$<INSTALL_INTERFACE:${KF5_INCLUDE_INSTALL_DIR}/KWayland/Server>")

target_link_libraries(KF5WaylandServer
    PUBLIC Qt5::Gui
    PRIVATE
        Wayland::Server
        EGL::EGL
)

if(IS_ABSOLUTE "${KF5_INCLUDE_INSTALL_DIR}")
  target_include_directories(KF5WaylandServer INTERFACE "$<INSTALL_INTERFACE:${KF5_INCLUDE_INSTALL_DIR}>" )
else()
  target_include_directories(KF5WaylandServer INTERFACE "$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/${KF5_INCLUDE_INSTALL_DIR}>" )
endif()

set_target_properties(KF5WaylandServer PROPERTIES VERSION   ${KWAYLAND_VERSION_STRING}
                                                 SOVERSION ${KWAYLAND_SOVERSION}
                                                 EXPORT_NAME WaylandServer
)

install(TARGETS KF5WaylandServer EXPORT KF5WaylandTargets ${KF5_INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/KWayland/Server/kwaylandserver_export.h
  blur_interface.h
  contrast_interface.h
  buffer_interface.h
  clientconnection.h
  compositor_interface.h
  datadevice_interface.h
  datadevicemanager_interface.h
  dataoffer_interface.h
  datasource_interface.h
  display.h
  dpms_interface.h
  fakeinput_interface.h
  global.h
  idle_interface.h
  keyboard_interface.h
  outputdevice_interface.h
  outputchangeset.h
  outputconfiguration_interface.h
  outputmanagement_interface.h
  output_interface.h
  pointer_interface.h
  plasmashell_interface.h
  plasmawindowmanagement_interface.h
  qtsurfaceextension_interface.h
  region_interface.h
  resource.h
  seat_interface.h
  server_decoration_interface.h
  shadow_interface.h
  shell_interface.h
  slide_interface.h
  subcompositor_interface.h
  surface_interface.h
  textinput_interface.h
  touch_interface.h
  xdgshell_interface.h
  DESTINATION ${KF5_INCLUDE_INSTALL_DIR}/KWayland/Server COMPONENT Devel
)

include(ECMGeneratePriFile)
ecm_generate_pri_file(BASE_NAME KWaylandServer LIB_NAME KF5WaylandServer DEPS "core" FILENAME_VAR PRI_FILENAME INCLUDE_INSTALL_DIR ${KDE_INSTALL_INCLUDEDIR_KF5})
install(FILES ${PRI_FILENAME}
        DESTINATION ${ECM_MKSPECS_INSTALL_DIR})
