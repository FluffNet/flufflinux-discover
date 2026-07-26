function(add_unit_test name)
    add_executable(${name} ${ARGN})
    add_test(${name} dbus-run-session ${CMAKE_BINARY_DIR}/bin/${name})
    ecm_mark_as_test(${name})
    target_link_libraries(${name}
        Discover::Common
        Qt::Test
        Qt::Core
        ${EXTRA_LIBS}
    )
endfunction()

# Shared logging category for backends which don't declare their own ones
ecm_qt_declare_logging_category(libdiscover-backend-logging-category_SRCS
    HEADER libdiscover_backend_debug.h
    IDENTIFIER LIBDISCOVER_BACKEND_LOG
    CATEGORY_NAME org.kde.plasma.libdiscover.backend
    DESCRIPTION "libdiscover (backend)"
    EXPORT DISCOVER
)

add_library(libdiscover-backend-logging-category STATIC
    ${libdiscover-backend-logging-category_SRCS}
)

set_property(TARGET libdiscover-backend-logging-category PROPERTY POSITION_INDEPENDENT_CODE ON)

target_link_libraries(libdiscover-backend-logging-category
    PRIVATE
        Qt::Core
)

add_subdirectory(FlatpakBackend)
