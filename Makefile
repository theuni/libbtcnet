#Placeholder Makefile

LIB_OBJS  = src/connection.o
LIB_OBJS += src/handler.o
LIB_OBJS += src/interface.o
LIB_OBJS += src/resolve.o
LIB_OBJS += src/directconn.o
LIB_OBJS += src/dnsconn.o
LIB_OBJS += src/bareconn.o
LIB_OBJS += src/connectionbase.o
LIB_OBJS += src/message.o
LIB_OBJS += src/proxyconn.o
LIB_OBJS += src/bareproxy.o
LIB_OBJS += src/eventtypes.o
LIB_OBJS += src/listener.o
LIB_OBJS += src/resolveonly.o
LIB_OBJS += src/incomingconn.o
LIB_OBJS += src/event.o
LIB_OBJS += src/base32.o

MULTINET_OBJS  = tests/multinet.o

OBJS = $(LIB_OBJS) $(MULTINET_OBJS)

LIBBTCNET=libbtcnet.a

LIBS=$(LIBBTCNET)

MULTINET=multinet
PROGS=$(MULTINET)

AR=ar
CXX=c++
CPPFLAGS=-Iinclude -I.
CXXFLAGS=-O2 -g -Wall -Wextra -Wno-unused-parameter
LDFLAGS=-levent_core -levent_pthreads -levent_extra

CPPFLAGS += -std=c++11

all: $(LIBS) $(PROGS)


%.o: %.cpp
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@
	@echo CXX: $<

$(LIBBTCNET): $(LIB_OBJS)
	@$(AR) cru $@ $^
	@echo AR:  $@

$(MULTINET): $(MULTINET_OBJS) $(LIBBTCNET)
	@$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $@
	@echo LD:  $@

clean:
	-rm -f $(OBJS) $(LIBS) $(PROGS)

tidy:
	@clang-tidy $(LIB_OBJS:.o=.cpp) -- $(CPPFLAGS)
