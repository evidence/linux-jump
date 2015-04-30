.PHONY: all clean

all:
	$(MAKE) -C jumplib
	$(MAKE) -C progsrc

clean:
	$(MAKE) -C jumplib clean
	$(MAKE) -C progsrc clean
