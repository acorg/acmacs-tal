# -*- Makefile -*-
# ======================================================================

TARGETS = \
  $(DIST)/tal

SOURCES = \
  settings.cc tree.cc time-series.cc tal.cc clades.cc hz-sections.cc json-export.cc coloring.cc \
  json-import.cc import-export.cc \
  draw-aa-transitions.cc aa-transition.cc aa-transition-20200915.cc \
  newick.cc draw-tree.cc \
  layout.cc html-export.cc draw.cc antigenic-maps.cc dash-bar.cc tal-data.cc legend.cc title.cc

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

install: $(TARGETS)
	$(call install_all,$(AD_PACKAGE_NAME))

test: install $(DIST)/tal
	test/test
.PHONY: test

# ----------------------------------------------------------------------

$(DIST)/tal: $(patsubst %.cc,$(BUILD)/%.o,$(SOURCES)) | $(DIST)
	$(call echo_link_exe,$@)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS) $(AD_RPATH)

# ======================================================================
### Local Variables:
### eval: (if (fboundp 'eu-rename-buffer) (eu-rename-buffer))
### End:
