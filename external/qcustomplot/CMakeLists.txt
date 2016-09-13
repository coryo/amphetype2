set(CMAKE_AUTOMOC ON)

aux_source_directory(. QCUSTOMPLOT_SRC)
add_library(qcustomplot STATIC ${QCUSTOMPLOT_SRC})
qt5_use_modules(qcustomplot Widgets PrintSupport)