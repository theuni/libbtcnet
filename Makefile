#Placeholder Makefile

LIB_OBJS  = src/connection.o
LIB_OBJS += src/handler.o
LIB_OBJS += src/message.o
LIB_OBJS += src/proxy.o
LIB_OBJS += src/callbacks.o

HELPER_OBJS  = helpers/simplenode.o
HELPER_OBJS += helpers/simplenodemanager.o
HELPER_OBJS += helpers/simpleaddrman.o
HELPER_OBJS += helpers/simpleconnman.o

HELPER_OBJS += helpers/threadedaddrman.o
HELPER_OBJS += helpers/threadedconnman.o
HELPER_OBJS += helpers/threadednode.o

SAMPLE_THREADED_OBJS = samples/threaded.o
SAMPLE_UNTHREADED_OBJS = samples/unthreaded.o

OBJS = $(LIB_OBJS) $(HELPER_OBJS) $(SAMPLE_THREADED_OBJS) $(SAMPLE_UNTHREADED_OBJS)

LIBBTCNET=libbtcnet.a
LIBHELPERS=libbtcnet-helpers.a

LIBS=$(LIBBTCNET) $(LIBHELPERS)

SAMPLE_UNTHREADED=sample-unthreaded
SAMPLE_THREADED=sample-threaded
PROGS=$(SAMPLE_UNTHREADED) $(SAMPLE_THREADED)

AR=ar
CXX=g++
CPPFLAGS=-Iinclude -I.
CXXFLAGS=-O2 -g -fPIC -pedantic -Wall -Wextra
LDFLAGS=-levent_core -levent_pthreads -levent_extra

#Use one or the other here. If c++11, there's no need for boost.
CPPFLAGS += -DUSE_CXX11 -std=c++11
#BOOST_LIBS=-lboost_system -lboost_thread

all: $(LIBS) $(PROGS)


%.o: %.cpp
	@$(CXX) $(CLANG_FLAGS) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
	@echo CXX: $<
$(LIBBTCNET): $(LIB_OBJS)
	@$(AR) cru $@ $^
	@echo AR:  $@

$(LIBHELPERS): $(HELPER_OBJS)
	@$(AR) cru $@ $^
	@echo AR:  $@

$(SAMPLE_UNTHREADED): $(SAMPLE_UNTHREADED_OBJS) $(LIBHELPERS) $(LIBBTCNET)
	@$(CXX) $(CXXFLAGS) $(LDFLAGS) $(BOOST_LIBS) $^ -o $@
	@echo LD:  $@

$(SAMPLE_THREADED): $(SAMPLE_THREADED_OBJS) $(LIBHELPERS) $(LIBBTCNET)
	@$(CXX) $(CXXFLAGS) $(LDFLAGS) $(BOOST_LIBS) $^ -o $@
	@echo LD:  $@
	
clean:
	-rm -f $(OBJS) $(LIBS) $(PROGS)
