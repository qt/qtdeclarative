# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Ensures that the provided uri matches the URI requirements. Write the result to the result
# variable.
function(_qt_internal_is_qml_uri_valid result uri)
    if("${uri}" MATCHES "^[a-zA-Z].*")
        set(${result} TRUE PARENT_SCOPE)
    else()
        set(${result} FALSE PARENT_SCOPE)
    endif()
endfunction()

# Ensures that the provided uri matches the URI requirements. Errors out if the URI is malformed.
function(_qt_internal_require_qml_uri_valid uri)
    _qt_internal_is_qml_uri_valid(ok "${uri}")
    if(NOT ok)
        message(FATAL_ERROR "URI must start with letter. Please specify a valid URI for ${target}.")
    endif()
endfunction()
