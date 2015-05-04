.PHONY: all clean

all:
	$(MAKE) -C jumplib
	$(MAKE) -C apps

clean:
	$(MAKE) -C jumplib clean
	$(MAKE) -C apps clean
