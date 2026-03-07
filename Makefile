TARGET      	:=  $(notdir $(CURDIR))
BUILD       	:=  build
LIBBUTANO   	:=  butano/butano
PYTHON      	:=  python3
SOURCES     	:=  src src/game butano/common/src
INCLUDES    	:=  include include/game ../libsavgba/include butano/common/include butano/butano/include
DATA        	:=  data
GRAPHICS    	:=  graphics graphics/common_palettes butano/common/graphics
AUDIO       	:=  audio audio/sounds butano/common/audio
AUDIOBACKEND	:=  maxmod
AUDIOTOOL		:=
DMGAUDIO    	:=  dmg_audio butano/common/dmg_audio
DMGAUDIOBACKEND	:=  default
ROMTITLE    	:=  DOOM2D
ROMCODE     	:=  ND2D
USERFLAGS   	:=	-std=gnu11 -DBN_CFG_SPRITES_MAX_ITEMS=128 -DRUN_TESTS -DD2D_DEBUG_ENABLE -DBN_CFG_SPRITES_MAX_SORT_LAYERS=8 -DBN_CFG_AUDIO_MAX_SOUND_CHANNELS=8 #-DBN_CFG_AUDIO_MIXING_RATE=BN_AUDIO_MIXING_RATE_10_KHZ
USERCXXFLAGS	:=
USERASFLAGS 	:=
USERLDFLAGS 	:=  -Wl,--print-memory-usage
USERLIBDIRS 	:=
USERLIBS    	:=
DEFAULTLIBS 	:=
STACKTRACE		:=
USERBUILD   	:=
EXTTOOL     	:=  @$(PYTHON) -B tools/build_info.py --compiler=$(CXX);

#---------------------------------------------------------------------------------------------------------------------
# Export absolute butano path:
#---------------------------------------------------------------------------------------------------------------------
ifndef LIBBUTANOABS
	export LIBBUTANOABS	:=	$(realpath $(LIBBUTANO))
endif

#---------------------------------------------------------------------------------
# This rule links in binary data with the .wad extension
#---------------------------------------------------------------------------------
%.wad.o %_wad.h :	%.wad
#---------------------------------------------------------------------------------
		@echo $(notdir $<)
		@$(bin2o)

#---------------------------------------------------------------------------------------------------------------------
# Include main makefile:
#---------------------------------------------------------------------------------------------------------------------
include $(LIBBUTANOABS)/butano.mak