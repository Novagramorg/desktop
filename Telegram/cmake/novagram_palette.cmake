# Novagram: the app's blue palette lives in lib_ui, a separate submodule, so
# it is kept in novagram_palette.diff and applied to the checkout here.

find_package(Git REQUIRED)

get_filename_component(novagram_lib_ui_loc ${CMAKE_CURRENT_LIST_DIR}/../lib_ui ABSOLUTE)
set(novagram_palette_diff ${CMAKE_CURRENT_LIST_DIR}/novagram_palette.diff)

execute_process(
    COMMAND ${GIT_EXECUTABLE} apply --reverse --check ${novagram_palette_diff}
    WORKING_DIRECTORY ${novagram_lib_ui_loc}
    RESULT_VARIABLE novagram_palette_applied
    OUTPUT_QUIET
    ERROR_QUIET
)
if (NOT novagram_palette_applied EQUAL 0)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} apply ${novagram_palette_diff}
        WORKING_DIRECTORY ${novagram_lib_ui_loc}
        RESULT_VARIABLE novagram_palette_result
        ERROR_VARIABLE novagram_palette_error
    )
    if (NOT novagram_palette_result EQUAL 0)
        message(FATAL_ERROR
            "The Novagram palette does not apply to lib_ui:\n"
            "${novagram_palette_error}"
            "Update Telegram/cmake/novagram_palette.diff for the new lib_ui.")
    endif()
    message(STATUS "Applied the Novagram palette to lib_ui.")
endif()
