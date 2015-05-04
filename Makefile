.PHONY: all clean

all:
	$(MAKE) -C build
	$(MAKE) -C apps

clean:
	$(MAKE) -C build clean
	$(MAKE) -C apps clean
