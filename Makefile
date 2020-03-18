# -*- Makefile -*-
# ======================================================================

TARGETS = \
  $(DIST)/tal

SOURCES = \
  tal.cc tal-data.cc tree.cc aa-transition.cc \
  draw.cc layout.cc coloring.cc draw-tree.cc time-series.cc clades.cc title.cc legend.cc dash-bar.cc draw-aa-transitions.cc \
  hz-sections.cc \
  settings.cc \
  import-export.cc json-export.cc json-import.cc newick.cc html-export.cc

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
	$(call symbolic_link,$(DIST)/tal,$(AD_BIN))
	if [ ! -f $(AD_SHARE)/conf/tal.json ]; then mkdir -p $(AD_SHARE)/conf; ln -sf $(abspath conf/tal.json) $(AD_SHARE)/conf; fi
	if [ ! -f $(AD_SHARE)/doc/tal-conf.org ]; then mkdir -p $(AD_SHARE)/doc; ln -sf $(abspath doc/tal-conf.org) $(AD_SHARE)/doc; fi

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
