CXX = mipsel-linux-gnu-g++

CXXFLAGS = -O2 \
           -fPIC \
           -Wa,-KPIC \
           -fno-stack-protector \
           -fno-use-cxa-atexit \
           -fno-exceptions \
           -fno-rtti \
           -D_GLIBCXX_USE_CXX11_ABI=0 \
           -U_FORTIFY_SOURCE \
           -D_FORTIFY_SOURCE=0 \
           -I./inc \
           -I./openimp/include

SYSROOT = ./vender-sysroot

LDFLAGS = -nodefaultlibs \
          -nostartfiles \
          -Wl,-z,max-page-size=0x1000 \
          -Wl,-z,common-page-size=0x1000 \
          -Wl,-e,__start \
          -L$(SYSROOT)/lib \
          -L./lib \
          -Wl,-rpath-link,$(SYSROOT)/lib \
          -limp \
          -lalog \
          -ldl \
          $(SYSROOT)/lib/librt.so.0 \
          $(SYSROOT)/lib/libm.so.0 \
          $(SYSROOT)/lib/libc.so.0 \
          $(SYSROOT)/lib/libstdc++.so.6 \
          $(SYSROOT)/lib/libgcc_s.so.1 \
          -Wl,--dynamic-linker,/lib/ld-uClibc.so.0

ifeq ($(DEBUG),1)
    CXXFLAGS += -g
endif

TARGET = main
SRCS = entry.S main.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)
