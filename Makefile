CC=gcc
CCFLAGS=-Wall -Wextra -fPIE -pie -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3 -Wformat -Wformat=2 -Wimplicit-fallthrough -fstack-clash-protection -fstack-protector-strong 
BUILDDIR=./build
SRCDIR=./src
LIBS=-lpthread
objects=$(addprefix $(BUILDDIR)/, main.o app.o dse.o kiss.o ringbuffer.o serial.o yframe.o iface/kiss.o)

dirs:
	mkdir -p $(BUILDDIR)/iface

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | dirs
	$(CC) -c -o $@ $< $(CCFLAGS)

dvtnc: $(objects)
	$(CC) -o $@ $^ $(CCFLAGS) $(LIBS)

test_rbuffer: $(addprefix $(BUILDDIR)/, test_rbuffer.o ringbuffer.o)
	$(CC) -o $@ $^ $(CCFLAGS) $(LIBS)


test_yframe: $(addprefix $(BUILDDIR)/, test_yframe.o yframe.o)
	$(CC) -o $@ $^ $(CCFLAGS) $(LIBS)

all: dvtnc

.PHONY: clean

clean: 
	rm -f $(BUILDDIR)/*.o
	rm -f dvtnc 
	rm -f test_rbuffer
	rm -f test_yframe

tests: test_yframe test_rbuffer
	./test_yframe
	./test_rbuffer

symbols:
	objdump -tC $(BUILDDIR)/*.o

debug: CCFLAGS += -g
debug: clean dvtnc

findleaks: CCFLAGS += -fsanitize=leak -fsanitize=address -fno-omit-frame-pointer -fno-common -g
findleaks: clean dvtnc
	ASAN_OPTIONS=verbosity=1:detect_stack_use_after_return=1:detect_leaks=1 ./dvtnc -s /dev/ttyUSB2 -k 8001
