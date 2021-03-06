########################################################
# Test WaylandConnectionThread
########################################################
set( testWaylandConnectionThread_SRCS
        test_wayland_connection_thread.cpp
    )
add_executable(testWaylandConnectionThread ${testWaylandConnectionThread_SRCS})
target_link_libraries( testWaylandConnectionThread Qt5::Test Qt5::Gui KF5::WaylandClient KF5::WaylandServer Wayland::Client Wayland::Server)
add_test(kwayland-testWaylandConnectionThread testWaylandConnectionThread)
ecm_mark_as_test(testWaylandConnectionThread)

########################################################
# Test WaylandRegistry
########################################################
set( testWaylandRegistry_SRCS
        test_wayland_registry.cpp
    )
add_executable(testWaylandRegistry ${testWaylandRegistry_SRCS})
target_link_libraries( testWaylandRegistry Qt5::Test Qt5::Gui KF5::WaylandClient KF5::WaylandServer Wayland::Client Wayland::Server)
add_test(kwayland-testWaylandRegistry testWaylandRegistry)
ecm_mark_as_test(testWaylandRegistry)

########################################################
# Test WaylandFullscreenShell
########################################################
if(Wayland_VERSION VERSION_GREATER "1.4.0")
    set( testWaylandFullscreenShell_SRCS
            test_wayland_fullscreen_shell.cpp
        )
    add_executable(testWaylandFullscreenShell ${testWaylandFullscreenShell_SRCS})
    target_link_libraries( testWaylandFullscreenShell Qt5::Test KF5::WaylandClient Wayland::Client)
    add_test(kwayland-testWaylandFullscreenShell testWaylandFullscreenShell)
    ecm_mark_as_test(testWaylandFullscreenShell)
endif()

########################################################
# Test WaylandOutput
########################################################
set( testWaylandOutput_SRCS
        test_wayland_output.cpp
    )
add_executable(testWaylandOutput ${testWaylandOutput_SRCS})
target_link_libraries( testWaylandOutput Qt5::Test Qt5::Gui KF5::WaylandClient KF5::WaylandServer Wayland::Client Wayland::Server)
add_test(kwayland-testWaylandOutput testWaylandOutput)
ecm_mark_as_test(testWaylandOutput)

########################################################
# Test WaylandShell
########################################################
set( testWaylandShell_SRCS
        test_wayland_shell.cpp
    )
add_executable(testWaylandShell ${testWaylandShell_SRCS})
target_link_libraries( testWaylandShell Qt5::Test Qt5::Gui KF5::WaylandClient KF5::WaylandServer Wayland::Client Wayland::Server)
add_test(kwayland-testWaylandShell testWaylandShell)
ecm_mark_as_test(testWaylandShell)

########################################################
# Test WaylandSurface
########################################################
set( testWaylandSurface_SRCS
        test_wayland_surface.cpp
    )
add_executable(testWaylandSurface ${testWaylandSurface_SRCS})
target_link_libraries( testWaylandSurface Qt5::Test Qt5::Gui KF5::WaylandClient KF5::WaylandServer Wayland::Client Wayland::Server)
add_test(kwayland-testWaylandSurface testWaylandSurface)
ecm_mark_as_test(testWaylandSurface)

########################################################
# Test WaylandSeat
########################################################
if (HAVE_LINUX_INPUT_H)
    set( testWaylandSeat_SRCS
            test_wayland_seat.cpp
        )
    add_executable(testWaylandSeat ${testWaylandSeat_SRCS})
    target_link_libraries( testWaylandSeat Qt5::Test Qt5::Gui KF5::WaylandClient KF5::WaylandServer Wayland::Client Wayland::Server)
    add_test(kwayland-testWaylandSeat testWaylandSeat)
    ecm_mark_as_test(testWaylandSeat)
endif()

########################################################
# Test ShmPool
########################################################
set( testShmPool_SRCS
        test_shm_pool.cpp
    )
add_executable(testShmPool ${testShmPool_SRCS})
target_link_libraries( testShmPool Qt5::Test Qt5::Gui KF5::WaylandClient KF5::WaylandServer)
add_test(kwayland-testShmPool testShmPool)
ecm_mark_as_test(testShmPool)

########################################################
# Test KWin OutputManagement
########################################################
set( test_wayland_outputmanagement_SRCS
        test_wayland_outputmanagement.cpp
    )
add_executable(testWaylandOutputManagement ${test_wayland_outputmanagement_SRCS})
target_link_libraries( testWaylandOutputManagement Qt5::Test Qt5::Gui KF5::WaylandClient KF5::WaylandServer Wayland::Client)
add_test(kwayland-testWaylandOutputManagement testWaylandOutputManagement)
ecm_mark_as_test(testWaylandOutputManagement)

########################################################
# Test KWin OutputDevice
########################################################
set( test_wayland_outputdevice_SRCS
        test_wayland_outputdevice.cpp
    )
add_executable(testWaylandOutputDevice ${test_wayland_outputdevice_SRCS})
target_link_libraries( testWaylandOutputDevice Qt5::Test Qt5::Gui KF5::WaylandClient KF5::WaylandServer Wayland::Client)
add_test(kwayland-testWaylandOutputDevice testWaylandOutputDevice)
ecm_mark_as_test(testWaylandOutputDevice)

########################################################
# Test Compositor
########################################################
set( testCompositor_SRCS
        test_compositor.cpp
    )
add_executable(testCompositor ${testCompositor_SRCS})
target_link_libraries( testCompositor Qt5::Test Qt5::Gui KF5::WaylandClient KF5::WaylandServer)
add_test(kwayland-testCompositor testCompositor)
ecm_mark_as_test(testCompositor)

########################################################
# Test SubCompositor
########################################################
set( testSubCompositor_SRCS
        test_wayland_subcompositor.cpp
    )
add_executable(testSubCompositor ${testSubCompositor_SRCS})
target_link_libraries( testSubCompositor Qt5::Test Qt5::Gui KF5::WaylandClient KF5::WaylandServer)
add_test(kwayland-testSubCompositor testSubCompositor)
ecm_mark_as_test(testSubCompositor)


########################################################
# Test SubSurface
########################################################
set( testSubSurface_SRCS
        test_wayland_subsurface.cpp
    )
add_executable(testSubSurface ${testSubSurface_SRCS})
target_link_libraries( testSubSurface Qt5::Test Qt5::Gui KF5::WaylandClient KF5::WaylandServer Wayland::Client)
add_test(kwayland-testSubSurface testSubSurface)
ecm_mark_as_test(testSubSurface)

########################################################
# Test Region
########################################################
set( testRegion_SRCS
        test_wayland_region.cpp
    )
add_executable(testRegion ${testRegion_SRCS})
target_link_libraries( testRegion Qt5::Test Qt5::Gui KF5::WaylandClient KF5::WaylandServer)
add_test(kwayland-testRegion testRegion)
ecm_mark_as_test(testRegion)

########################################################
# Test Blur
########################################################
set( testBlur_SRCS
        test_wayland_blur.cpp
    )
add_executable(testBlur ${testBlur_SRCS})
target_link_libraries( testBlur Qt5::Test Qt5::Gui KF5::WaylandClient KF5::WaylandServer)
add_test(kwayland-testBlur testBlur)
ecm_mark_as_test(testBlur)

########################################################
# Test Contrast
########################################################
set( testContrast_SRCS
        test_wayland_contrast.cpp
    )
add_executable(testContrast ${testContrast_SRCS})
target_link_libraries( testContrast Qt5::Test Qt5::Gui KF5::WaylandClient KF5::WaylandServer Wayland::Client)
add_test(kwayland-testContrast testContrast)
ecm_mark_as_test(testContrast)

########################################################
# Test Slide
########################################################
set( testSlide_SRCS
        test_wayland_slide.cpp
    )
add_executable(testSlide ${testSlide_SRCS})
target_link_libraries( testSlide Qt5::Test Qt5::Gui KF5::WaylandClient KF5::WaylandServer Wayland::Client)
add_test(kwayland-testSlide testSlide)
ecm_mark_as_test(testSlide)

########################################################
# Test Window Management
########################################################
set( testWindowmanagement_SRCS
        test_wayland_windowmanagement.cpp
    )
add_executable(testWindowmanagement ${testWindowmanagement_SRCS})
target_link_libraries( testWindowmanagement Qt5::Test Qt5::Gui KF5::WaylandClient KF5::WaylandServer Wayland::Client)
add_test(kwayland-testWindowmanagement testWindowmanagement)
ecm_mark_as_test(testWindowmanagement)

########################################################
# Test DataSource
########################################################
set( testDataSource_SRCS
        test_datasource.cpp
    )
add_executable(testDataSource ${testDataSource_SRCS})
target_link_libraries( testDataSource Qt5::Test Qt5::Gui KF5::WaylandClient KF5::WaylandServer Wayland::Client)
add_test(kwayland-testDataSource testDataSource)
ecm_mark_as_test(testDataSource)

########################################################
# Test DataDevice
########################################################
set( testDataDevice_SRCS
        test_datadevice.cpp
    )
add_executable(testDataDevice ${testDataDevice_SRCS})
target_link_libraries( testDataDevice Qt5::Test Qt5::Gui KF5::WaylandClient KF5::WaylandServer Wayland::Client)
add_test(kwayland-testDataDevice testDataDevice)
ecm_mark_as_test(testDataDevice)

########################################################
# Test ServerSideDecoration
########################################################
set( testServerSideDecoration_SRCS
        test_server_side_decoration.cpp
    )
add_executable(testServerSideDecoration ${testServerSideDecoration_SRCS})
target_link_libraries( testServerSideDecoration Qt5::Test Qt5::Gui KF5::WaylandClient KF5::WaylandServer Wayland::Client)
add_test(kwayland-testServerSideDecoration testServerSideDecoration)
ecm_mark_as_test(testServerSideDecoration)

########################################################
# Test Drag'N'Drop
########################################################
set( testDragAndDrop_SRCS
        test_drag_drop.cpp
    )
add_executable(testDragAndDrop ${testDragAndDrop_SRCS})
target_link_libraries( testDragAndDrop Qt5::Test Qt5::Gui KF5::WaylandClient KF5::WaylandServer Wayland::Client)
add_test(kwayland-testDragAndDrop testDragAndDrop)
ecm_mark_as_test(testDragAndDrop)

########################################################
# Test PlasmaShell
########################################################
set( testPlasmaShell_SRCS
        test_plasmashell.cpp
    )
add_executable(testPlasmaShell ${testPlasmaShell_SRCS})
target_link_libraries( testPlasmaShell Qt5::Test Qt5::Gui KF5::WaylandClient KF5::WaylandServer Wayland::Client)
add_test(kwayland-testPlasmaShell testPlasmaShell)
ecm_mark_as_test(testPlasmaShell)

########################################################
# Test Idle
########################################################
set( testIdle_SRCS
        test_idle.cpp
    )
add_executable(testIdle ${testIdle_SRCS})
target_link_libraries( testIdle Qt5::Test Qt5::Gui KF5::WaylandClient KF5::WaylandServer)
add_test(kwayland-testIdle testIdle)
ecm_mark_as_test(testIdle)

########################################################
# Test Shadow
########################################################
set( testShadow_SRCS
        test_shadow.cpp
    )
add_executable(testShadow ${testShadow_SRCS})
target_link_libraries( testShadow Qt5::Test Qt5::Gui KF5::WaylandClient KF5::WaylandServer)
add_test(kwayland-testShadow testShadow)
ecm_mark_as_test(testShadow)

########################################################
# Test FakeInput
########################################################
if (HAVE_LINUX_INPUT_H)
    set( testFakeInput_SRCS
            test_fake_input.cpp
        )
    add_executable(testFakeInput ${testFakeInput_SRCS})
    target_link_libraries( testFakeInput Qt5::Test Qt5::Gui KF5::WaylandClient KF5::WaylandServer)
    add_test(kwayland-testFakeInput testFakeInput)
    ecm_mark_as_test(testFakeInput)
endif()

########################################################
# Test PlasmaWindowModel
########################################################
if (HAVE_LINUX_INPUT_H)
    set( testPlasmaWindowModel_SRCS
            test_plasma_window_model.cpp
        )
    add_executable(testPlasmaWindowModel ${testPlasmaWindowModel_SRCS})
    target_link_libraries( testPlasmaWindowModel Qt5::Test Qt5::Gui KF5::WaylandClient KF5::WaylandServer)
    add_test(kwayland-testPlasmaWindowModel testPlasmaWindowModel)
    ecm_mark_as_test(testPlasmaWindowModel)
endif()

########################################################
# Test TextInput
########################################################
set( testTextInput_SRCS
        test_text_input.cpp
    )
add_executable(testTextInput ${testTextInput_SRCS})
target_link_libraries( testTextInput Qt5::Test Qt5::Gui KF5::WaylandClient KF5::WaylandServer)
add_test(kwayland-testTextInput testTextInput)
ecm_mark_as_test(testTextInput)

########################################################
# Test Error
########################################################
set( testError_SRCS
        test_error.cpp
    )
add_executable(testError ${testError_SRCS})
target_link_libraries( testError Qt5::Test Qt5::Gui KF5::WaylandClient KF5::WaylandServer Wayland::Client)
add_test(kwayland-testError testError)
ecm_mark_as_test(testError)

########################################################
# Test Selection
########################################################
set( testSelection_SRCS
        test_selection.cpp
    )
add_executable(testSelection ${testSelection_SRCS})
target_link_libraries( testSelection Qt5::Test Qt5::Gui KF5::WaylandClient KF5::WaylandServer Wayland::Client)
add_test(kwayland-testSelection testSelection)
ecm_mark_as_test(testSelection)

########################################################
# Test XdgShellV5
########################################################
set( testXdgShellV5_SRCS
      test_xdg_shell.cpp
      )
add_executable(testXdgShellV5 ${testXdgShellV5_SRCS})
target_link_libraries( testXdgShellV5 Qt5::Test Qt5::Gui KF5::WaylandServer KF5::WaylandClient Wayland::Client)
add_test(kwayland-testXdgShellV5 testXdgShellV5)
ecm_mark_as_test(testXdgShellV5)

