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

