
TOPDIR = ../../..
include $(TOPDIR)/Config.mk

ifneq (,$(BUILDDIR))
BUILDDIR_uxenstor := $(shell mkdir -p $(BUILDDIR) && cd $(BUILDDIR) && pwd)
SET_OBJECT_ROOT := set OBJECT_ROOT=$(call dospath,$(BUILDDIR_uxenstor))&
OBJDIR_uxenstor := $(BUILDDIR_uxenstor)/$(shell pwd -W | sed 's,^[a-z]:/,,i')
else
BUILDDIR_uxenstor := .
SET_OBJECT_ROOT :=
OBJDIR_uxenstor := .
endif

SET_STAMPINF_DATE := set STAMPINF_DATE=$(shell TZ=GMT date +%m/%d/%Y)&
SET_WDK8_PATH := set WDK8_PATH=$(call dospath,$(TOOLSDIR)/cross-windows/wdk8)&

SET_UXENDIRS := set UXEN_DISTDIR_VM_SUPPORT=$(call dospath,$(DISTDIR_VM_SUPPORT))& set UXEN_SDKDIR_INCLUDE=$(call dospath,$(SDKDIR_include))& set UXEN_SDKDIR_LIB=$(call dospath,$(SDKDIR_lib))&

OUTDIR_uxenstor := $(OBJDIR_uxenstor)/$(OBJDIR_ddk)

.PHONY: all
all: $(OUTDIR_uxenstor)/uxenstor.sys

# NOTE: no space after set foo=xxx since cmd otherwise includes the space in foo
.PHONY: $(OUTDIR_uxenstor)/uxenstor.sys
$(OUTDIR_uxenstor)/uxenstor.sys: $(OBJDIR_uxenstor)/build_info.h
	@mkdir -p $(@D)
	$(_W)echo WinDDK build $@
	$(_V)cmd /c "set UXEN_BUILD_INFO=\"\"\"../../build_info.h\"\"\"& set MAKEFLAGS=& $(SET_STAMPINF_DATE) $(SET_WDK8_PATH) $(SET_UXENDIRS) $(WINDDK_DIR)\bin\setenv.bat $(WINDDK_DIR)\ $(DDKENV) $(UXEN_TARGET_VM_SUPPORT_ARCH_NAME) $(UXEN_TARGET_VM_SUPPORT_OS) no_oacr & cd /D $$(pwd -W) & $(SET_OBJECT_ROOT) $(WINDDK_BUILD) -cZ -jpath $(call dospath,$(OBJDIR_uxenstor))"
	$(_W)echo Signing - $@
	$(_V)cmd //c $(UXEN_WINDOWS_SIGN) $@ || { rm -f $@; false; }

.PHONY: $(OUTDIR_uxenstor)/uxenstor.cat
$(OUTDIR_uxenstor)/uxenstor.cat: $(OUTDIR_uxenstor)/uxenstor.sys
	$(_W)echo Signing - $@
	$(_V)cmd //c $(UXEN_WINDOWS_SIGN) $(call dospath, $@) || { rm -f $@; false; }

.PHONY: $(OBJDIR_uxenstor)/build_info.h
$(OBJDIR_uxenstor)/build_info.h:
	$(_W)echo Generating - $(@F)
	@mkdir -p $(@D)
	@( echo "#define UXEN_DRIVER_FILEVERSION1 " $$(git log --pretty=format:%cd --date=short -n 1 | sed 's/\(....\)-..-../\1/'); \
	   echo "#define UXEN_DRIVER_FILEVERSION2 " $$(git log --pretty=format:%cd --date=short -n 1 | sed 's/....-\(..\)-\(..\)/\1\2/'); \
	   echo "#define UXEN_DRIVER_FILEVERSION3 " $$(( $$(git log --pretty=format:%ct -n 1) / 65536 )); \
	   echo "#define UXEN_DRIVER_FILEVERSION4 " $$(( $$(git log --pretty=format:%ct -n 1) % 65536 )); \
	   echo "#define UXEN_DRIVER_VERSION_CHANGESET \""$$(git log --pretty=format:%H -n 1 && git diff --quiet || echo -dirty)"\""; \
	   ) >$@

INSTALL_FILES = uxenstor.sys
INSTALL_FILES += uxenstor.cat
INSTALL_FILES += uxenstor.inf
INSTALL_FILES += uxenstor.pdb

INSTALL_DIR = $(DISTDIR_VM_SUPPORT)/uxenstor

_install_banner: $(INSTALL_DIR)/.exists
	$(_W)echo Installing from $(abspath $(OUTDIR_uxenstor)) to $(INSTALL_DIR)

VPATH += $(OUTDIR_uxenstor)
$(patsubst %,install_%,$(INSTALL_FILES)): install_%: _install_banner
$(patsubst %,install_%,$(INSTALL_FILES)): install_%: $(OUTDIR_uxenstor)/%
	$(_W)echo Installing -- $(<F)
	$(_V)$(call install_data,$(<),$(INSTALL_DIR))

dist: $(patsubst %,install_%,$(INSTALL_FILES))

clean::
	@$(if $(BUILDDIR),rm -rf $(BUILDDIR),:)
$(BUILDDIR:%=x)clean::
	@rm -rf obj{chk,fre}_$(UXEN_TARGET_VM_SUPPORT_OS)_$(UXEN_TARGET_VM_SUPPORT_ARCH)
	@rm -f build{chk,fre}_$(UXEN_TARGET_VM_SUPPORT_OS)_$(UXEN_TARGET_VM_SUPPORT_ARCH).{log,err,wrn}
