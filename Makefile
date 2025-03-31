CC=gcc
CCFLAGS=-Wall -Wextra -fPIE -pie -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3 -Wformat -Wformat=2 -Wimplicit-fallthrough -fstack-clash-protection -fstack-protector-strong 
BUILDDIR=./build
SRCDIR=./src
LIBS=-lpthread
objects=$(addprefix $(BUILDDIR)/, main.o ringbuffer.o serial.o yframe.o)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -c -o $@ $< $(CCFLAGS)

dvtnc: $(objects)
	$(CC) -o $@ $^ $(CCFLAGS) $(LIBS)

all: dvtnc

.PHONY: clean

clean: 
	rm -f $(BUILDDIR)/*.o
	rm -f dvtnc 

tests: dvtnc
	./dvtnc --run-tests

symbols:
	objdump -tC $(BUILDDIR)/*.o

debug: CCFLAGS += -g
debug: clean dvtnc

findleaks: CCFLAGS += -fsanitize=leak -fsanitize=address -fno-omit-frame-pointer -fno-common -g
findleaks: clean dvtnc
	ASAN_OPTIONS=verbosity=1:detect_stack_use_after_return=1:detect_leaks=1 ./dvtnc -v
