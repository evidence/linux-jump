#*********************************************************
#  Platform dependent rules and defines...
#*********************************************************

ARCH = linux
ARCH_FLAGS  = -DLINUX -DCachepages=1024
CC  = gcc
CCC = gcc

SRC	= ../jumpsrc 
CFLAGS	= -I./${SRC} $(ARCH_FLAGS) -Wall -Wextra
OBJS 	= comm.o init.o mem.o syn.o tools.o exit.o

VPATH   = ./$(SRC)

TARGET 	= libjia.a

%.d: %.c
	@echo "Creating $@..."
	@$(SHELL) -ec "$(CC) -MM $(CPPFLAGS) $< | sed ' s/$*\.o/& $@/g' > $@"

$(TARGET):$(OBJS)
	ar rv $(TARGET) $?

all:$(TARGET)

clean:
	rm -f *.[od] $(TARGET)

include $(OBJS:.o=.d) 
