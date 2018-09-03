# MIT License
#
# Copyright (c) 2018 Andres Amaya Garcia
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# Source directories
LIBDIR = library
INCDIR = include

# Source file names
SRCS =  fetch.cpp     \
		memory.cpp    \
		processor.cpp \
		regfile.cpp   \
		decode.cpp    \
		execute.cpp   \
		alu.cpp       \
		lsu.cpp       \
		branch.cpp    \
		misc.cpp      \
		stats.cpp     \
		simulator.cpp

# Code format tool
CFMT ?= clang-format-6.0

# Output files
OBJS = $(addprefix $(LIBDIR)/,$(SRCS:.cpp=.o))
DEPS = $(addprefix $(LIBDIR)/,$(SRCS:.cpp=.d))
EXEC = simulator

# Code format configuration file
CFMTCFG = .clang-format

# Compiler options
OPT_LEVEL ?= 2
CFLAGS    ?= -O2 -Wall -Wextra -Werror -ansi -pedantic -std=c++14 -I./$(INCDIR)
LDFLAGS   ?=
DEPFLAGS  ?= -MT $@ -MMD -MP -MF $*.Td
CFMTFLAGS ?= -i -style=file

all: $(OBJS) $(EXEC)

$(EXEC): $(OBJS)
	@echo "  LD    $@"
	@$(CXX) $(LDFLAGS) $(OBJS) -o $@

%.o: %.cpp
%.o: %.cpp %.d
	@echo "  CC    $< -> $@"
	@$(CXX) $(CFLAGS) $(DEPFLAGS) -c $< -o $@
	@mv -f $*.Td $*.d

%.d: ;

# Convenience target to format all the code
ALL_SRCS =  $(shell find $(INCDIR)/simulator -regex '.*\.h') \
			$(addprefix $(LIBDIR)/,$(SRCS))
format: $(CFMTCFG)
	@for src_file in $(ALL_SRCS); do      \
		echo "  FMT   $$src_file" ;       \
		$(CFMT) $(CFMTFLAGS) $$src_file ; \
	done

# Include the header dependency Makefiles (if any)
-include $(DEPS)

.PRECIOUS: %.d

.PHONY: clean all

clean:
	$(RM) $(LIBDIR)/*.o $(LIBDIR)/*.d $(LIBDIR)/*.Td $(EXEC)
