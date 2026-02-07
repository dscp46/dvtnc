CC=gcc
CCFLAGS=-Wall -Wextra -fPIE -pie -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3 -Wformat -Wformat=2 -Wimplicit-fallthrough -fstack-clash-protection -fstack-protector-strong 
BUILDDIR=./build
SRCDIR=./src
LIBS=-lpthread -lz
objects=$(addprefix $(BUILDDIR)/, main.o app.o dse.o kiss.o ringbuffer.o serial.o yframe.o)

dvtnc: $(objects)
	$(CC) -o $@ $^ $(CCFLAGS) $(LIBS)

.PHONY: dirs
dirs:
	test -d $(BUILDDIR)/iface || mkdir -p $(BUILDDIR)/iface

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | dirs
	$(CC) -c -o $@ $< $(CCFLAGS)

$(BUILDDIR)/iface/%.o: $(SRCDIR)/iface/%.c | dirs
	$(CC) -c -o $@ $< $(CCFLAGS)

test_rbuffer: $(addprefix $(BUILDDIR)/, test_rbuffer.o ringbuffer.o)
	$(CC) -o $@ $^ $(CCFLAGS) $(LIBS)


test_yframe: $(addprefix $(BUILDDIR)/, test_yframe.o yframe.o)
	$(CC) -o $@ $^ $(CCFLAGS) $(LIBS)

test_kiss: $(addprefix $(BUILDDIR)/, test_kiss.o iface/kiss.o)
	$(CC) -o $@ $^ $(CCFLAGS) $(LIBS)

all: dvtnc

.PHONY: clean
clean: 
	find $(BUILDDIR) -name '*.o' -delete
	rm -f dvtnc test_rbuffer test_yframe test_kiss

tests: CCFLAGS += -g
tests: test_yframe test_rbuffer test_kiss
	./test_yframe
	./test_rbuffer
	./test_kiss

symbols:
	objdump -tC $(BUILDDIR)/*.o

debug: CCFLAGS += -g
debug: clean dvtnc

findleaks: CCFLAGS += -fsanitize=leak -fsanitize=address -fno-omit-frame-pointer -fno-common -g
findleaks: clean dvtnc
	ASAN_OPTIONS=verbosity=1:detect_stack_use_after_return=1:detect_leaks=1 ./dvtnc -s /dev/ttyUSB2 -k 8001
