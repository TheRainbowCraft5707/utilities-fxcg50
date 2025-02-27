#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
# Set toolchain location in an environment var for future use, this will change
# to use a system environment var in the future.
#---------------------------------------------------------------------------------
ifeq ($(strip $(FXCGSDK)),)
export FXCGSDK := $(abspath ../../)
endif

include $(FXCGSDK)/toolchain/prizm_rules


#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
TARGET		:=	Utilities
BUILD		:=	build
SOURCES		:=	src
DATA		:=	data  
INCLUDES	:=

#---------------------------------------------------------------------------------
# git version controlling mechanism
#---------------------------------------------------------------------------------
ifeq ($(OS),Windows_NT) #force version and timestamp defines to update.
$(shell cmd /C 'type nul >> $(CURDIR)/src/versionProvider.cpp') 
GIT_VERSION = $(shell cmd /C 'git describe --abbrev=4 --dirty --always')
else
$(shell touch $(CURDIR)/src/versionProvider.cpp)
GIT_VERSION = $(shell sh -c 'git describe --abbrev=4 --dirty --always')
endif
GIT_TIMESTAMP += "$(shell git log --pretty=format:'%aD' -1)"

#---------------------------------------------------------------------------------
# options for code and add-in generation
#---------------------------------------------------------------------------------

MKG3AFLAGS := -n basic:Utilities -i uns:../unselected.bmp -i sel:../selected.bmp

# Optional: add -flto to CFLAGS and LDFLAGS to enable link-time optimization
# (LTO). Doing so will usually allow the compiler to generate much better code
# (smaller and/or faster), but may expose bugs in your code that don't cause
# any trouble without LTO enabled.
CFLAGS	= -Os -Wall $(MACHDEP) $(INCLUDE) -ffunction-sections -fdata-sections \
		  -D__GIT_VERSION=\"$(GIT_VERSION)\" -D__GIT_TIMESTAMP=\"$(GIT_TIMESTAMP)\"
CXXFLAGS	=	$(CFLAGS) -fno-exceptions

LDFLAGS	= $(MACHDEP) -T$(FXCGSDK)/toolchain/prizm.x -Wl,-static -Wl,-gc-sections

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS	:=	 -lc -lfxcg -lgcc

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:=

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

#---------------------------------------------------------------------------------
# automatically build a list of object files for our project
#---------------------------------------------------------------------------------
CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
sFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.S)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
					$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) \
					$(sFILES:.s=.o) $(SFILES:.S=.o)

#---------------------------------------------------------------------------------
# build a list of include paths
#---------------------------------------------------------------------------------
export INCLUDE	:=	$(foreach dir,$(INCLUDES), -iquote $(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD) -I$(LIBFXCG_INC)

#---------------------------------------------------------------------------------
# build a list of library paths
#---------------------------------------------------------------------------------
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib) \
					-L$(LIBFXCG_LIB)

export OUTPUT	:=	$(CURDIR)/$(TARGET)
.PHONY: all clean

#---------------------------------------------------------------------------------
all: $(BUILD)
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

$(BUILD):
	@mkdir $@

#---------------------------------------------------------------------------------
export CYGWIN := nodosfilewarning
clean:
	$(call rmdir,$(BUILD))
	$(call rm,$(OUTPUT).bin)
	$(call rm,$(OUTPUT).g3a)

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).g3a: $(OUTPUT).bin
$(OUTPUT).bin: $(OFILES)


-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
