# This file is part of Telegram Desktop,
# the official desktop application for the Telegram messaging service.
#
# For license and copyright information please follow this link:
# https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL

add_executable(test_suspicious_names)
init_non_host_target(test_suspicious_names "(tests)")

target_precompile_headers(test_suspicious_names PRIVATE ${src_loc}/tests/test_suspicious_names_pch.h)
nice_target_sources(test_suspicious_names ${src_loc}
PRIVATE
    data/data_peer_suspicious_name.cpp
    data/data_peer_suspicious_name.h
    tests/test_suspicious_names.cpp
    tests/test_suspicious_names_pch.h
)

target_include_directories(test_suspicious_names
PRIVATE
    ${src_loc}
)

target_link_libraries(test_suspicious_names
PRIVATE
    desktop-app::lib_base
)

set_target_properties(test_suspicious_names PROPERTIES
    EXCLUDE_FROM_ALL TRUE
    EXCLUDE_FROM_DEFAULT_BUILD TRUE
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test_suspicious_names
)
