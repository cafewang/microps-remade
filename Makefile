APPS = 

DRIVERS = driver/dummy.o \
          driver/loopback.o \

OBJS = util.o \
       net.o \
       ip.o \
       icmp.o \

TESTS = test/step0.exe \
        test/step1.exe \
        test/step2.exe \
        test/step3.exe \
        test/step4.exe \
        test/step7.exe \
        test/step8.exe \
        test/step9.exe \
        test/step11.exe \

UNIT_TESTS = unit_test/ip_test.exe \
             unit_test/intr_test.exe \

CFLAGS := $(CFLAGS) -g -W -Wall -Wno-unused-parameter -iquote . -DHEXDUMP

ifeq ($(shell uname),Linux)
  # Linux specific settings
  BASE = platform/linux
  CFLAGS := $(CFLAGS) -pthread -iquote $(BASE)
  OBJS := $(OBJS) $(BASE)/intr.o
endif

ifeq ($(shell uname),Darwin)
  # macOS specific settings
endif

.SUFFIXES:
.SUFFIXES: .c .o

.PHONY: all clean run_unit_tests

all: $(APPS) $(TESTS)

$(APPS): %.exe : %.o $(OBJS) $(DRIVERS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TESTS): %.exe : %.o $(OBJS) $(DRIVERS) test/test.h
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(UNIT_TESTS): %.exe : %.o $(OBJS) $(DRIVERS) unit_test/unit_test.h
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(APPS) $(APPS:.exe=.o) $(OBJS) $(DRIVERS) $(TESTS) $(TESTS:.exe=.o) $(UNIT_TESTS) $(UNIT_TESTS:.exe=.o)

run_unit_tests: $(UNIT_TESTS)
	@for test in $(UNIT_TESTS); do \
		echo "Running $$test..."; \
		./$$test || exit 1; \
	done