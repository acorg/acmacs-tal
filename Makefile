# -*- Makefile -*-
# ======================================================================

TARGETS = \
  $(TAL_LIB) \
  $(DIST)/tal

TAL_SOURCES = \
  settings.cc tree.cc time-series.cc clades.cc hz-sections.cc json-export.cc coloring.cc \
  json-import.cc import-export.cc \
  draw-aa-transitions.cc aa-transition.cc aa-transition-20200915.cc aa-transition-20210503.cc \
  newick.cc draw-tree.cc \
  layout.cc html-export.cc draw.cc antigenic-maps.cc dash-bar.cc tal-data.cc legend.cc title.cc

TAL_LIB_MAJOR = 1
TAL_LIB_MINOR = 0
TAL_LIB_NAME = libtal
TAL_LIB = $(DIST)/$(call shared_lib_name,$(TAL_LIB_NAME),$(TAL_LIB_MAJOR),$(TAL_LIB_MINOR))

# ----------------------------------------------------------------------

all: install

CONFIGURE_CAIRO = 1
include $(ACMACSD_ROOT)/share/Makefile.config

ACMACSD_LIBS = \
  $(AD_LIB)/$(call shared_lib_name,libacmacsbase,1,0) \
  $(AD_LIB)/$(call shared_lib_name,liblocationdb,1,0) \
  $(AD_LIB)/$(call shared_lib_name,libacmacsvirus,1,0) \
  $(AD_LIB)/$(call shared_lib_name,libacmacschart,2,0) \
  $(AD_LIB)/$(call shared_lib_name,libacmacswhoccdata,1,0) \
  $(AD_LIB)/$(call shared_lib_name,libhidb,5,0) \
  $(AD_LIB)/$(call shared_lib_name,libseqdb,3,0) \
  $(AD_LIB)/$(call shared_lib_name,libacmacsdraw,1,0) \
  $(AD_LIB)/$(call shared_lib_name,libacmacsmapdraw,2,0) \
  $(CXX_LIBS)

LDLIBS = $(ACMACSD_LIBS) $(CAIRO_LIBS) $(XZ_LIBS)

# ----------------------------------------------------------------------

install: install-headers $(TARGETS)
	$(call install_lib,$(TAL_LIB))
	$(call install_all,$(AD_PACKAGE_NAME))

test: install $(DIST)/tal
	test/test
.PHONY: test

# ----------------------------------------------------------------------

$(TAL_LIB): $(patsubst %.cc,$(BUILD)/%.o,$(TAL_SOURCES)) | $(DIST) install-headers
	$(call echo_shared_lib,$@)
	$(call make_shared_lib,$(TAL_LIB_NAME),$(TAL_LIB_MAJOR),$(TAL_LIB_MINOR)) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(DIST)/%: $(BUILD)/%.o | $(TAL_LIB)
	$(call echo_link_exe,$@)
	$(CXX) $(LDFLAGS) -o $@ $^ $(TAL_LIB) $(LDLIBS) $(AD_RPATH)

$(BUILD)/%.o: cc/$(ALGLIB)/%.cpp | $(BUILD) install-headers
	$(call echo_compile,$<)
	$(CXX) -c -o $@ $(abspath $<)

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
