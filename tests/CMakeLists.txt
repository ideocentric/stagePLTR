# Unit tests (Qt Test + CTest). Each test is its own executable linking the
# shared stageplt_core library. Run them with: ctest --output-on-failure

# Tests that need the device catalog read it from the source tree rather than
# the compiled Qt resource, so they don't depend on the app's resource bundle.
set(STAGEPLT_ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets/plot")

function(add_unit_test name)
    qt_add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE stageplt_core Qt6::Test)
    target_compile_definitions(${name} PRIVATE
        "STAGEPLT_ASSETS_DIR=\"${STAGEPLT_ASSETS_DIR}\"")
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_unit_test(test_ports)
add_unit_test(test_documentinfo)
add_unit_test(test_pageconfig)
add_unit_test(test_devicecatalog)
add_unit_test(test_inputlist)

# These need a QApplication; run them headless via the offscreen plugin.
add_unit_test(test_stagescene)
add_unit_test(test_undo)
set_tests_properties(test_stagescene test_undo PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen")