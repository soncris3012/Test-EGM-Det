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
    src/mainwindow.ui
    src/mainwindow.cpp src/mainwindow.h
    src/benchmarkworker.cpp src/benchmarkworker.h
    src/evaluator.cpp src/evaluator.h
    src/reliabilityanalyzer.cpp src/reliabilityanalyzer.h
    src/reliabilitychartwidget.cpp src/reliabilitychartwidget.h
    src/dataloader.cpp src/dataloader.h
    src/imageview.cpp src/imageview.h
    src/types.h
)

target_link_libraries(egm-det-benchmark PRIVATE
    Qt6::Widgets Qt6::Concurrent Qt6::PrintSupport
)
target_include_directories(egm-det-benchmark PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
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
    qt_add_executable(reliability-tests tests/reliability_tests.cpp src/reliabilityanalyzer.cpp src/reliabilityanalyzer.h)
    target_link_libraries(reliability-tests PRIVATE Qt6::Widgets)
    target_compile_definitions(reliability-tests PRIVATE QT_NO_KEYWORDS)
    add_test(NAME reliability-tests COMMAND reliability-tests)
    qt_add_executable(dataloader-tests tests/dataloader_tests.cpp src/dataloader.cpp src/dataloader.h src/types.h)
    target_link_libraries(dataloader-tests PRIVATE Qt6::Widgets)
    target_compile_definitions(dataloader-tests PRIVATE QT_NO_KEYWORDS)
    add_test(NAME dataloader-tests COMMAND dataloader-tests)
    qt_add_executable(single-pair-tests tests/single_pair_tests.cpp src/benchmarkworker.cpp src/benchmarkworker.h src/dataloader.cpp src/dataloader.h src/reliabilityanalyzer.cpp src/reliabilityanalyzer.h src/types.h)
    target_link_libraries(single-pair-tests PRIVATE Qt6::Widgets Qt6::Concurrent)
    target_compile_definitions(single-pair-tests PRIVATE QT_NO_KEYWORDS)
    add_test(NAME single-pair-tests COMMAND single-pair-tests)
endif()
