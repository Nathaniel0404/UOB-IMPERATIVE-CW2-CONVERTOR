
# Find the OS platform using the uname command.
Linux := $(findstring Linux, $(shell uname -s))
MacOS := $(findstring Darwin, $(shell uname -s))
Windows := $(findstring NT, $(shell uname -s))

# Specify what typing 'make' on its own will compile
default: converter

# For native Windows, add the .exe extension
ifdef Windows

%: %.c
	clang -Dtest_$@ -std=c11 -Wall -pedantic -g $@.c -o $@.exe  -fsanitize=undefined -fsanitize=address `pkg-config --libs --cflags sdl2`

# For Linux/MacOS/Windows LS, include the advanced debugging options
else

%: %.c
	clang -Dtest_$@ -std=c11 -Wall -pedantic -g $@.c -o $@ \
	    -fsanitize=undefined -fsanitize=address `pkg-config --libs --cflags sdl2`

endif
