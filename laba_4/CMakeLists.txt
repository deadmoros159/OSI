cmake_minimum_required(VERSION 3.10)
project(OS_Lab4 C)

set(CMAKE_C_STANDARD 99)

# Опции компиляции
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

# Указываем, где искать заголовочные файлы
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# ============ ДИНАМИЧЕСКИЕ БИБЛИОТЕКИ ============

# Библиотека для e - реализация 1 (с математической библиотекой)
add_library(e_impl1 SHARED ${CMAKE_CURRENT_SOURCE_DIR}/e_impl1.c)
target_link_libraries(e_impl1 m)

# Библиотека для e - реализация 2
add_library(e_impl2 SHARED ${CMAKE_CURRENT_SOURCE_DIR}/e_impl2.c)

# Библиотека для площади - реализация 1
add_library(area_impl1 SHARED ${CMAKE_CURRENT_SOURCE_DIR}/area_impl1.c)

# Библиотека для площади - реализация 2
add_library(area_impl2 SHARED ${CMAKE_CURRENT_SOURCE_DIR}/area_impl2.c)

# Устанавливаем выходную директорию для библиотек в корень проекта
set_target_properties(e_impl1 e_impl2 area_impl1 area_impl2 PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# ============ ПРОГРАММА 1 (статическая линковка) ============

# Создаем объектные файлы для статической линковки
add_library(e_impl1_obj OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/e_impl1.c)
target_link_libraries(e_impl1_obj m)

add_library(area_impl1_obj OBJECT ${CMAKE_CURRENT_SOURCE_DIR}/area_impl1.c)

# Создаем исполняемый файл для программы 1
add_executable(prog1 ${CMAKE_CURRENT_SOURCE_DIR}/prog1.c $<TARGET_OBJECTS:e_impl1_obj> $<TARGET_OBJECTS:area_impl1_obj>)
target_link_libraries(prog1 m)

# ============ ПРОГРАММА 2 (динамическая загрузка) ============

# Для динамической загрузки нужна библиотека dl
add_executable(prog2 ${CMAKE_CURRENT_SOURCE_DIR}/prog2.c)
target_link_libraries(prog2 dl)

# ============ КОМАНДЫ ДЛЯ ЗАПУСКА ============

# Команда для запуска prog1
add_custom_target(run_prog1
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/prog1
    DEPENDS prog1
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Запуск программы 1 (статическая линковка)"
)

# Команда для запуска prog2 с правильным путем к библиотекам
add_custom_target(run_prog2
    COMMAND ${CMAKE_COMMAND} -E env LD_LIBRARY_PATH=${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/prog2
    DEPENDS prog2 e_impl1 e_impl2 area_impl1 area_impl2
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Запуск программы 2 (динамическая загрузка)"
)

# ============ ИНФОРМАЦИЯ ============

message(STATUS "======================================")
message(STATUS "Лабораторная работа №4")
message(STATUS "Динамические библиотеки")
message(STATUS "======================================")
message(STATUS "Текущая директория: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Директория сборки: ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "Библиотеки будут в: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Программы будут в: ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "======================================")
message(STATUS "Для запуска:")
message(STATUS "  make run_prog1  - программа 1 (статическая)")
message(STATUS "  make run_prog2  - программа 2 (динамическая)")
message(STATUS "Или вручную:")
message(STATUS "  cd ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "  ${CMAKE_CURRENT_BINARY_DIR}/prog1")
message(STATUS "  LD_LIBRARY_PATH=${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}/prog2")
message(STATUS "======================================")