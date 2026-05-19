cmake_minimum_required(VERSION 3.20)
project(LLM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_SUPPRESS_REGENERATION ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ui)

set(CMAKE_PREFIX_PATH "C:/Qt/Qt5.15.1/5.15.1/msvc2019_64")

find_package(Qt5 REQUIRED COMPONENTS Widgets Concurrent)

include(FetchContent)
FetchContent_Declare(
	googletest
	URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(CORE_MATH_INCLUDE_PATH "include/core/math")
set(CORE_MATH_SOURCE_PATH "source/core/math")

add_library(core_math 
	${CORE_MATH_INCLUDE_PATH}/CMatrix.h
	${CORE_MATH_SOURCE_PATH}/CMatrix.cpp
	${CORE_MATH_INCLUDE_PATH}/CMatrix_ops.h
	${CORE_MATH_SOURCE_PATH}/CMatrix_ops.cpp
	${CORE_MATH_INCLUDE_PATH}/CActivations.h
	${CORE_MATH_SOURCE_PATH}/CActivations.cpp
)
target_include_directories(core_math PUBLIC include)

set(CORE_TOKENIZER_INCLUDE_PATH "include/core/tokenizer")
set(CORE_TOKENIZER_SOURCE_PATH "source/core/tokenizer")

add_library(core_tokenizer
	${CORE_TOKENIZER_INCLUDE_PATH}/CTokenizer.h
	${CORE_TOKENIZER_SOURCE_PATH}/CTokenizer.cpp
	${CORE_TOKENIZER_INCLUDE_PATH}/CBPETokenizer.h
	${CORE_TOKENIZER_SOURCE_PATH}/CBPETokenizer.cpp
)
target_include_directories(core_tokenizer PUBLIC include)
target_link_libraries(core_tokenizer PUBLIC core_math)

set(CORE_MODEL_INCLUDE_PATH "include/core/model")
set(CORE_MODEL_SOURCE_PATH "source/core/model")

set(CORE_TRAINING_INCLUDE_PATH "include/core/training")
set(CORE_TRAINING_SOURCE_PATH "source/core/training")

add_library(core_model
	${CORE_MODEL_INCLUDE_PATH}/CEmbedding.h
	${CORE_MODEL_SOURCE_PATH}/CEmbedding.cpp
	${CORE_MODEL_INCLUDE_PATH}/CAttention.h
	${CORE_MODEL_SOURCE_PATH}/CAttention.cpp
	${CORE_MODEL_INCLUDE_PATH}/CFFLayer.h
	${CORE_MODEL_SOURCE_PATH}/CFFLayer.cpp
	${CORE_MODEL_INCLUDE_PATH}/CLayerNorm.h
	${CORE_MODEL_SOURCE_PATH}/CLayerNorm.cpp
	${CORE_MODEL_INCLUDE_PATH}/CTransformerBlock.h
	${CORE_MODEL_SOURCE_PATH}/CTransformerBlock.cpp

	${CORE_TRAINING_INCLUDE_PATH}/CLoss.h
	${CORE_TRAINING_SOURCE_PATH}/CLoss.cpp
	${CORE_TRAINING_INCLUDE_PATH}/COptimizer.h
	${CORE_TRAINING_SOURCE_PATH}/COptimizer.cpp
	${CORE_TRAINING_INCLUDE_PATH}/CTrainer.h
	${CORE_TRAINING_SOURCE_PATH}/CTrainer.cpp
)
target_include_directories(core_model PUBLIC include)
find_package(Threads REQUIRED)
target_link_libraries(core_model PUBLIC core_math Threads::Threads)

set(BIBLIO_INCLUDE_PATH "include/biblio")
set(BIBLIO_SOURCE_PATH "source/biblio")

add_library(biblio 
	${BIBLIO_INCLUDE_PATH}/CArticle.h
	${BIBLIO_INCLUDE_PATH}/CArticleSource.h
	${BIBLIO_INCLUDE_PATH}/CArxivSource.h
	${BIBLIO_SOURCE_PATH}/CArxivSource.cpp
	${BIBLIO_INCLUDE_PATH}/CBibAnalyzer.h
	${BIBLIO_SOURCE_PATH}/CBibAnalyzer.cpp
	${BIBLIO_INCLUDE_PATH}/CPubMedSource.h
	${BIBLIO_SOURCE_PATH}/CPubMedSource.cpp
	${BIBLIO_INCLUDE_PATH}/CSemanticScholarSource.h
	${BIBLIO_SOURCE_PATH}/CSemanticScholarSource.cpp
)
target_include_directories(biblio PUBLIC include)
target_link_libraries(biblio PUBLIC core_model core_tokenizer)

add_executable(LLM_Train source/train.cpp)
target_include_directories(LLM_Train PRIVATE include)
target_link_libraries(LLM_Train PRIVATE biblio)

add_executable(BPEDemo source/bpe_demo.cpp)
target_include_directories(BPEDemo PRIVATE include)
target_link_libraries(BPEDemo PRIVATE core_tokenizer)

add_executable(Benchmark source/benchmark.cpp)
target_include_directories(Benchmark PRIVATE include)
target_link_libraries(Benchmark PRIVATE biblio)

add_executable(${PROJECT_NAME} WIN32
	source/main.cpp
	include/ui/MainWindow.h
	source/ui/MainWindow.cpp
	ui/MainWindow.ui

	include/ui/ArticleItemWidget.h
	source/ui/ArticleItemWidget.cpp
	include/ui/SavedArticlesManager.h
	source/ui/SavedArticlesManager.cpp

	include/ui/ExportDialog.h
	source/ui/ExportDialog.cpp

	resources/resources.qrc
)

target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} PRIVATE biblio Qt5::Widgets Qt5::Concurrent)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND "C:/Qt/Qt5.15.1/5.15.1/msvc2019_64/bin/windeployqt.exe"
            --no-translations
            $<TARGET_FILE:${PROJECT_NAME}>
    COMMENT "Running windeployqt..."
)

enable_testing()
add_subdirectory(tests)

set_target_properties(gtest gtest_main gmock gmock_main PROPERTIES FOLDER "ThirdParty")
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})