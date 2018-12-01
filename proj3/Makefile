.SUFFIXES: .cpp .o

CXX=g++

SRCDIR=src/
INC=include/
LIBS=lib/

# main source file
TARGET_SRC:=$(SRCDIR)main.cpp
TARGET_OBJ:=$(SRCDIR)main.o
STATIC_LIB:=$(LIBS)libbpt.a
DYNAMIC_LIB:=$(LIBS)libbpt.so

# Include more files if you write another source file.
SRCS_FOR_LIB:= \
	$(SRCDIR)univ.cpp \
	$(SRCDIR)global_vars.cpp \
	$(SRCDIR)update.cpp \
	$(SRCDIR)delete.cpp \
	$(SRCDIR)insert.cpp \
	$(SRCDIR)deadlock.cpp \
	$(SRCDIR)trx.cpp \
	$(SRCDIR)lock.cpp \
	$(SRCDIR)bpt_ext.cpp \
	$(SRCDIR)table.cpp \
	$(SRCDIR)file.cpp \
	$(SRCDIR)buf.cpp 

OBJS_FOR_LIB:=$(SRCS_FOR_LIB:.cpp=.o)

CXXFLAGS+= -g -std=c++14 -fPIC -I $(INC)

TARGET=main

all: $(TARGET)

$(TARGET): $(TARGET_OBJ) $(STATIC_LIB)
	$(CXX) $(CXXFLAGS) $< -o $@ -L $(LIBS) -lbpt

%.o:	%.cpp
	$(CXX) $(CXXFLAGS) $^ -c -o $@

clean:
	rm -rf $(TARGET) $(TARGET_OBJ) $(OBJS_FOR_LIB) $(LIBS)*

$(DYNAMIC_LIB):	$(OBJS_FOR_LIB)
	g++ -shared -Wl,-soname,libbpt.so -o $@ $(OBJS_FOR_LIB)

$(STATIC_LIB):	$(OBJS_FOR_LIB)
	ar cr $@ $^

