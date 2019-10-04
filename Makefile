# -*- Makefile -*-
# ======================================================================

TARGETS = \
  $(DIST)/tal

SOURCES = \
  tal.cc tree.cc \
  export.cc json-export.cc newick.cc

# ----------------------------------------------------------------------

all: install

CONFIGURE_CAIRO = 1
include $(ACMACSD_ROOT)/share/Makefile.config

ACMACSD_LIBS = \
  $(AD_LIB)/$(call shared_lib_name,libacmacsbase,1,0) \
  $(AD_LIB)/$(call shared_lib_name,liblocationdb,1,0) \
  $(AD_LIB)/$(call shared_lib_name,libacmacsvirus,1,0) \
  $(AD_LIB)/$(call shared_lib_name,libacmacschart,2,0) \
  $(AD_LIB)/$(call shared_lib_name,libhidb,5,0) \
  $(AD_LIB)/$(call shared_lib_name,libseqdb,3,0) \
  $(AD_LIB)/$(call shared_lib_name,libacmacsdraw,1,0) \
  $(AD_LIB)/$(call shared_lib_name,libacmacsmapdraw,2,0) \
  $(CXX_LIBS)

LDLIBS = $(ACMACSD_LIBS) $(CAIRO_LIBS) $(XZ_LIBS)

# ----------------------------------------------------------------------

install: $(TARGETS)
	$(call symbolic_link,$(DIST)/tal,$(AD_BIN))

test: install $(DIST)/sigp
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
