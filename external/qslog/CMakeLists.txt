set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)

aux_source_directory(. QSLOG_SRC)
add_library(qslog STATIC ${QSLOG_SRC})
qt5_use_modules(qslog Core)