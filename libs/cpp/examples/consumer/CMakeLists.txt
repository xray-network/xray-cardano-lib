cmake_minimum_required(VERSION 3.28)
project(CardanoInstalledConsumer LANGUAGES CXX)

find_package(CardanoLib CONFIG REQUIRED COMPONENTS lib)
add_executable(cardano_installed_consumer main.cpp)
target_compile_features(cardano_installed_consumer PRIVATE cxx_std_23)
target_link_libraries(cardano_installed_consumer PRIVATE cardano::lib)
