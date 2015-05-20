# Set -DARCH_X86 or -DARCH_ARM depending on your specific hardware
# Add -DNFS to enable NFS
ARCH_FLAGS  = -D_GNU_SOURCE -DCachepages=1024 -DARCH_X86

export CC  = gcc
export CFLAGS	= -I../include $(ARCH_FLAGS) -Wall -Wextra -pedantic -std=c99 -O0 -g

.PHONY: all clean lib apps

all: lib apps

lib:
	$(MAKE) -C build

apps:
	$(MAKE) -C apps

clean:
	$(MAKE) -C build clean
	$(MAKE) -C apps clean

