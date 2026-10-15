# Unit tests (sheredom/utest.h). Reuses the engine sources via the `cae_core`
# library and registers the binary with CTest.

add_executable(cae_tests test.cpp)

# Some engine code prints extra diagnostics when TEST_ACTIVE is defined.
target_compile_definitions(cae_tests PRIVATE TEST_ACTIVE)

target_link_libraries(cae_tests PRIVATE cae_core)

add_test(NAME cae_tests COMMAND cae_tests)
