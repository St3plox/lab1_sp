cmake_minimum_required(VERSION 3.16) # Минимальная версия системы сборки CMake.
project(masked_line_edit LANGUAGES CXX) # Имя проекта; CXX означает язык C++.

set(CMAKE_CXX_STANDARD 17) # Используем синтаксис и возможности C++17.
set(CMAKE_CXX_STANDARD_REQUIRED ON) # Не заменять C++17 более старым стандартом.
set(CMAKE_AUTOMOC ON) # CMake сам запускает moc для классов Qt с Q_OBJECT.

# Widgets нужен приложению, Test — автоматическим проверкам.
find_package(Qt6 REQUIRED COMPONENTS Widgets Test)

# Собираем наш класс отдельно: его можно подключить и к другому окну Qt.
add_library(masked_line_edit maskedlineedit.cpp maskedlineedit.h)
# PUBLIC означает, что потребители библиотеки тоже получат зависимость Widgets.
target_link_libraries(masked_line_edit PUBLIC Qt6::Widgets)
# Позволяет писать #include "maskedlineedit.h" в остальных файлах проекта.
target_include_directories(masked_line_edit PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(mask_demo main.cpp) # Программа с демонстрационным окном.
target_link_libraries(mask_demo PRIVATE masked_line_edit) # Подключаем наш класс.

# Регистрируем тесты, чтобы их запускала команда ctest.
enable_testing()
add_executable(mask_tests tests.cpp) # Отдельная программа с проверками.
target_link_libraries(mask_tests PRIVATE masked_line_edit Qt6::Test)
add_test(NAME mask_tests COMMAND mask_tests)
# Тесты создают виджет, но не требуют настоящего монитора или открытого окна.
set_tests_properties(mask_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
