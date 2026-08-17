# CMakeList.txt: CMake-Projektdatei der obersten Ebene. Führen Sie hier die globale Konfiguration aus,
# und schließen Sie Unterprojekte ein.
#
cmake_minimum_required (VERSION 3.10)

project ("ThermalControl")

# Schließen Sie Unterprojekte ein.
add_subdirectory ("ThermalControl")
