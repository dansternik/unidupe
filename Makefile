 # file: Makefile
 # --------------
 # Makefile for unidupe.
 #
 # -----------------------------------------------------------------
 #  MIT License
 #
 #  Copyright (c) 2017 dansternik (Dominique Piens)
 #
 #  Permission is hereby granted, free of charge, to any person obtaining a copy
 #  of this software and associated documentation files (the "Software"), to deal
 #  in the Software without restriction, including without limitation the rights
 #  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 #  copies of the Software, and to permit persons to whom the Software is
 #  furnished to do so, subject to the following conditions:
 #
 #  The above copyright notice and this permission notice shall be included in all
 #  copies or substantial portions of the Software.
 #
 #  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 #  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 #  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 #  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 #  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 #  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 #  SOFTWARE.

CXX = g++
CXXFLAGS = -g -Wall -pedantic -O0 -std=c++11 -MD
LD_FLAGS = -L/usr/lib/x86_64-linux-gnu/ -lcrypto -lssl
SOURCES = \
	  unidupe.cc \
	  FsNode.cc \
	  EditStep.cc \
	  FsTree.cc

LIB_OBJ = $(patsubst %.cc,%.o,$(patsubst %.S,%.o,$(SOURCES)))
LIB_DEP = $(patsubst %.o,%.d,$(LIB_OBJ))
LIB = unidupe.a

HEADERS = $(SOURCES:.cc=.h)
OBJECTS = $(SOURCES:.cc:.o)
TARGET = unidupe

default: $(TARGET)

unidupe: $(OBJECTS) $(LIB)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(OBJECTS) $(LD_FLAGS)
$(LIB) : $(LIB_OBJ)
	rm -f $@
	ar r $@ $^
	ranlib $@
-include $(LIB_DEP)

clean::
	@rm -f $(TARGET) $(LIB_OBJ) $(LIB_DEP) $(LIB)

