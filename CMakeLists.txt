cmake_minimum_required(VERSION 3.16)
project(NutritionTrackerAI LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Works with Qt 6 (recommended) or Qt 5
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Network)

add_executable(NutritionTrackerAI
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/ClaudeClient.h
    src/ClaudeClient.cpp
    src/FoodEntry.h
    src/FoodEntry.cpp
    src/Tracker.h
    src/Tracker.cpp
)

set_target_properties(NutritionTrackerAI PROPERTIES WIN32_EXECUTABLE ON)
target_link_libraries(NutritionTrackerAI PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network)
