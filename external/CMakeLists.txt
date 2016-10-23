add_subdirectory(qcustomplot)
add_subdirectory(sqlite)
add_subdirectory(sqlite3pp)
add_subdirectory(qslog)

set_target_properties(qcustomplot PROPERTIES FOLDER "3rd Party Libs")
set_target_properties(sqlite3 PROPERTIES FOLDER "3rd Party Libs")
set_target_properties(qslog PROPERTIES FOLDER "3rd Party Libs")
set_target_properties(sqlite3pp PROPERTIES FOLDER "3rd Party Libs")
