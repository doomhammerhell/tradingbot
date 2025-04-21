cmake_minimum_required(VERSION 3.15)
project(tradingbot VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(ENABLE_TESTS "Enable building tests" OFF)
option(ENABLE_DOCS "Enable building documentation" OFF)
option(ENABLE_OPTIMIZATION "Enable parameter optimization" ON)

# Add subdirectories
add_subdirectory(src)

# Find required packages
find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(Crow REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${EIGEN3_INCLUDE_DIR}
)

# Add source files
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "src/optimization/*.cpp"
)

# Add header files
file(GLOB_RECURSE HEADERS 
    "include/*.hpp"
    "include/optimization/*.hpp"
)

# Main executable
add_executable(tradingbot ${SOURCES} ${HEADERS})

# Optimization executable
if(ENABLE_OPTIMIZATION)
    add_executable(tradingbot_optimize
        src/main_optimize.cpp
        src/optimization/ParameterOptimizer.cpp
    )
    
    target_link_libraries(tradingbot_optimize
        PRIVATE
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        Eigen3::Eigen
        Crow::Crow
        OpenSSL::SSL
        OpenSSL::Crypto
        CURL::libcurl
    )
endif()

# Link libraries
target_link_libraries(tradingbot
    PRIVATE
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    Eigen3::Eigen
    Crow::Crow
    OpenSSL::SSL
    OpenSSL::Crypto
    CURL::libcurl
)

# Testing configuration
if(ENABLE_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.2
    )
    FetchContent_MakeAvailable(Catch2)
    
    # Add test executable
    add_executable(tradingbot_tests
        tests/strategies/EnsembleStrategyTest.cpp
        tests/optimization/ParameterOptimizerTest.cpp
        # Add more test files here
    )
    
    target_link_libraries(tradingbot_tests
        PRIVATE
        tradingbot
        Catch2::Catch2
    )
    
    include(CTest)
    include(Catch)
    catch_discover_tests(tradingbot_tests)
endif()

# Documentation configuration
if(ENABLE_DOCS)
    find_package(Doxygen REQUIRED)
    
    set(DOXYGEN_INPUT_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    
    set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/docs)
    set(DOXYGEN_MAINPAGE ${CMAKE_CURRENT_SOURCE_DIR}/README.md)
    set(DOXYGEN_PROJECT_NAME "Trading Bot")
    set(DOXYGEN_PROJECT_BRIEF "A professional C++ trading bot with multiple strategies")
    set(DOXYGEN_PROJECT_LOGO ${CMAKE_CURRENT_SOURCE_DIR}/docs/logo.png)
    set(DOXYGEN_USE_MDFILE_AS_MAINPAGE YES)
    set(DOXYGEN_GENERATE_TREEVIEW YES)
    set(DOXYGEN_CLASS_DIAGRAMS YES)
    set(DOXYGEN_HIDE_UNDOC_MEMBERS NO)
    set(DOXYGEN_HIDE_UNDOC_CLASSES NO)
    set(DOXYGEN_HIDE_FRIEND_COMPOUNDS NO)
    set(DOXYGEN_HIDE_IN_BODY_DOCS NO)
    set(DOXYGEN_SEARCHENGINE YES)
    set(DOXYGEN_SOURCE_BROWSER YES)
    set(DOXYGEN_INLINE_SOURCES NO)
    set(DOXYGEN_STRIP_CODE_COMMENTS YES)
    set(DOXYGEN_GENERATE_LATEX NO)
    set(DOXYGEN_CALL_GRAPH YES)
    set(DOXYGEN_CALLER_GRAPH YES)
    set(DOXYGEN_COLLABORATION_GRAPH YES)
    set(DOXYGEN_CLASS_GRAPH YES)
    set(DOXYGEN_INCLUDE_GRAPH YES)
    set(DOXYGEN_INCLUDED_BY_GRAPH YES)
    
    doxygen_add_docs(docs
        ${DOXYGEN_INPUT_DIRS}
        COMMENT "Generate documentation"
    )
endif()

# Install configuration
install(TARGETS tradingbot
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
)

# Add custom targets
add_custom_target(format
    COMMAND find ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/include
        -name "*.cpp" -o -name "*.hpp"
        | xargs clang-format -i
    COMMENT "Formatting source files"
)

add_custom_target(analyze
    COMMAND find ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/include
        -name "*.cpp" -o -name "*.hpp"
        | xargs clang-tidy
    COMMENT "Running static analysis"
) 