# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.10

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:


#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:


# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list


# Suppress display of executed commands.
$(VERBOSE).SILENT:


# A target that is always out of date.
cmake_force:

.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /mnt/c/Users/shivi/Desktop/Project/Code/IEGenLib-improve-computation-api

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /mnt/c/Users/shivi/Desktop/Project/Code/IEGenLib-improve-computation-api/build

# Utility rule file for rtest.

# Include the progress variables for this target.
include CMakeFiles/rtest.dir/progress.make

CMakeFiles/rtest: iegen_lib_test


iegen_lib_test: bin/iegenlib_t
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --blue --bold --progress-dir=/mnt/c/Users/shivi/Desktop/Project/Code/IEGenLib-improve-computation-api/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Run iegenlib tests"
	IEGEN_HOME=. src/iegenlib_t

rtest: CMakeFiles/rtest
rtest: iegen_lib_test
rtest: CMakeFiles/rtest.dir/build.make

.PHONY : rtest

# Rule to build all files generated by this target.
CMakeFiles/rtest.dir/build: rtest

.PHONY : CMakeFiles/rtest.dir/build

CMakeFiles/rtest.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/rtest.dir/cmake_clean.cmake
.PHONY : CMakeFiles/rtest.dir/clean

CMakeFiles/rtest.dir/depend:
	cd /mnt/c/Users/shivi/Desktop/Project/Code/IEGenLib-improve-computation-api/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /mnt/c/Users/shivi/Desktop/Project/Code/IEGenLib-improve-computation-api /mnt/c/Users/shivi/Desktop/Project/Code/IEGenLib-improve-computation-api /mnt/c/Users/shivi/Desktop/Project/Code/IEGenLib-improve-computation-api/build /mnt/c/Users/shivi/Desktop/Project/Code/IEGenLib-improve-computation-api/build /mnt/c/Users/shivi/Desktop/Project/Code/IEGenLib-improve-computation-api/build/CMakeFiles/rtest.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/rtest.dir/depend

