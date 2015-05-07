#*********************************************************
#  Platform dependent rules and defines...
#*********************************************************

ARCH_FLAGS  = -DLINUX -D_GNU_SOURCE -DCachepages=1024 -DX86
CC  = gcc

SRC	= ../jumpsrc 

.PHONY: clean

# Add -DNFS to CFLAGS to enable NFS
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
