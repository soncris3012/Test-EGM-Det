cmake_minimum_required(VERSION 3.21)
project(TestEGMDet VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets Concurrent PrintSupport)

qt_add_executable(egm-det-benchmark
    src/main.cpp
    src/mainwindow.cpp src/mainwindow.h
    src/benchmarkworker.cpp src/benchmarkworker.h
    src/evaluator.cpp src/evaluator.h
    src/dataloader.cpp src/dataloader.h
    src/imageview.cpp src/imageview.h
    src/types.h
)

target_link_libraries(egm-det-benchmark PRIVATE
    Qt6::Widgets Qt6::Concurrent Qt6::PrintSupport
)
target_compile_definitions(egm-det-benchmark PRIVATE QT_NO_KEYWORDS)

install(TARGETS egm-det-benchmark
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

include(CTest)
if(BUILD_TESTING)
    qt_add_executable(evaluator-tests tests/evaluator_tests.cpp src/evaluator.cpp src/evaluator.h src/types.h)
    target_link_libraries(evaluator-tests PRIVATE Qt6::Widgets)
    target_compile_definitions(evaluator-tests PRIVATE QT_NO_KEYWORDS)
    add_test(NAME evaluator-tests COMMAND evaluator-tests)
endif()
