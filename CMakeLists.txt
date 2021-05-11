cmake_minimum_required(VERSION 3.19)
project(FAR)


set(CMAKE_C_STANDARD 99)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)


add_executable(Client client.c)
add_executable(Serveur fonction/fserveur.c serveur.c)
target_link_libraries(Client Threads::Threads)
target_link_libraries(Serveur Threads::Threads)