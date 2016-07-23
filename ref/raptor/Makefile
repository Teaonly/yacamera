OBJS_DIR = ./obj

AR = ar
CC = g++
CFLAGS = -O2 -I.
LDFLAGS = -lm 

LIB_SRCS =
include build.mk
LIB_SRCS_ = ${LIB_SRCS:%.cc=$(OBJS_DIR)/%.o}
LIB_OBJS = ${LIB_SRCS_:%.cpp=$(OBJS_DIR)/%.o}

TARGET = raptor

$(OBJS_DIR)/%.o : %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJS_DIR)/%.o : %.cc
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJS_DIR)/%.o : %.cpp
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(LIB_OBJS)
	$(CC) -g -o $@ $(LIB_OBJS) $(LDFLAGS)

.PHONY: clean all
all: $(TARGET)
clean:
	rm -f $(TARGET)
	rm -rf $(OBJS_DIR)
