#*********************************************************
#  Platform dependent rules and defines...
#*********************************************************

# Set -DARCH_X86 or -DARCH_ARM depending on your specific hardware
# Add -DNFS to enable NFS
ARCH_FLAGS  = -D_GNU_SOURCE -DCachepages=1024 -DARCH_X86

CC  = gcc
CFLAGS	= -I../include $(ARCH_FLAGS) -Wall -Wextra 

TARGET 	= libjia.a
SRC	= ../src 

.PHONY: clean

OBJS 	= comm.o init.o mem.o syn.o tools.o exit.o

$(TARGET):$(OBJS)
	ar rv $(TARGET) $?

VPATH   = ./$(SRC)

%.d: %.c
	@echo "Creating $@..."
	@$(SHELL) -ec "$(CC) -MM $(CPPFLAGS) $< | sed ' s/$*\.o/& $@/g' > $@"
	include $(OBJS:.o=.d) 

clean:
	rm -f *.[od] $(TARGET)

