TARGET      := mister-companion-vita
TITLE       := MiSTer Companion Vita
TITLE_ID    := MCVITA001
BUILD       := build
SOURCES     := source
INCLUDES    := source
ICON        := icon0.png

PREFIX      := arm-vita-eabi
CC          := $(PREFIX)-gcc
CXX         := $(PREFIX)-g++
STRIP       := $(PREFIX)-strip

CFILES      := $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.c))
CPPFILES    := $(foreach dir,$(SOURCES),$(wildcard $(dir)/*.cpp))
OBJS        := $(addprefix $(BUILD)/,$(notdir $(CFILES:.c=.o))) $(addprefix $(BUILD)/,$(notdir $(CPPFILES:.cpp=.o)))
VPATH       := $(SOURCES)

CFLAGS      := -g -Wall -O2 -ffunction-sections -fdata-sections -D__vita__
CFLAGS      += $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir))
CFLAGS      += -I$(VITASDK)/arm-vita-eabi/include

CXXFLAGS    := $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++17

LDFLAGS     := -Wl,-q -Wl,--gc-sections -L$(VITASDK)/arm-vita-eabi/lib

LIBS        := -lssh2 -lssl -lcrypto
LIBS        += -lmbedtls -lmbedx509 -lmbedcrypto
LIBS        += -lvita2d -lpng -ljpeg -lz -lfreetype -lm

LIBS        += -lSceDisplay_stub
LIBS        += -lSceGxm_stub
LIBS        += -lSceCtrl_stub
LIBS        += -lSceTouch_stub
LIBS        += -lSceCommonDialog_stub
LIBS        += -lSceIme_stub
LIBS        += -lSceSysmodule_stub
LIBS        += -lSceNet_stub
LIBS        += -lSceNetCtl_stub
LIBS        += -lSceAppMgr_stub
LIBS        += -lSceAppUtil_stub
LIBS        += -lScePgf_stub
LIBS        += -lSceLibKernel_stub

.PHONY: all clean

all: $(TARGET).vpk

$(BUILD):
	@mkdir -p $@

$(BUILD)/%.o: %.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(TARGET).elf: $(OBJS)
	$(CXX) $(LDFLAGS) $^ $(LIBS) -o $@

$(TARGET).velf: $(TARGET).elf
	vita-elf-create $< $@

eboot.bin: $(TARGET).velf
	vita-make-fself -c $< $@

param.sfo:
	vita-mksfoex -s TITLE_ID=$(TITLE_ID) "$(TITLE)" $@

$(TARGET).vpk: eboot.bin param.sfo $(ICON)
	vita-pack-vpk -s param.sfo -b eboot.bin -a $(ICON)=sce_sys/icon0.png $@

clean:
	rm -rf $(BUILD) $(TARGET).elf $(TARGET).velf eboot.bin param.sfo $(TARGET).vpk

-include $(OBJS:.o=.d)