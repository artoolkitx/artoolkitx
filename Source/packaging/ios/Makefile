#
# Build file for creating DMG files.
#
# The DMG packager looks for a template.dmg.bz2 for using as its 
# DMG template. If it doesn't find one, it generates a clean one.
#
# If you create a DMG template, you should make one containing all
# the files listed in $(SOURCE_FILES) below, and arrange everything to suit
# your style. The contents of the files themselves does not matter, so
# they can be empty (they will be overwritten later). 
#
# Remko Troncon 
# http://el-tramo.be/about
#


################################################################################
# Customizable variables
################################################################################

DIST_NAME=MyApp
DIST_VERSION=0.1

SOURCE_DIR=src
SOURCE_FILES=MyApp.app README COPYING

TEMPLATE_DMG=template.dmg

################################################################################
# DMG building. No editing should be needed beyond this point.
################################################################################

MASTER_DMG=$(DIST_NAME) $(DIST_VERSION).dmg
WC_DMG=wc.dmg
WC_DIR=wc

.PHONY: all dist
all: dist

$(TEMPLATE_DMG): $(TEMPLATE_DMG).bz2
	bunzip2 -k $<

$(TEMPLATE_DMG).bz2: 
	@echo
	@echo --------------------- Generating empty template --------------------
	mkdir template
	hdiutil create -size 40m "$(TEMPLATE_DMG)" -srcfolder template -format UDRW -volname "$(DIST_NAME)" -quiet
	rmdir template
	bzip2 "$(TEMPLATE_DMG)"
	@echo

$(WC_DMG): $(TEMPLATE_DMG)
	cp $< $@

dist: $(WC_DMG) $(addprefix $(SOURCE_DIR)/,$(SOURCE_FILES))
	@echo
	@echo --------------------- Creating Disk Image --------------------
	mkdir -p $(WC_DIR)
	hdiutil attach "$(WC_DMG)" -noautoopen -quiet -mountpoint "$(WC_DIR)"
	for i in $(SOURCE_FILES); do  \
		rm -rf "$(WC_DIR)/$$i"; \
		ditto -rsrc "$(SOURCE_DIR)/$$i" "$(WC_DIR)/$$i"; \
	done
	hdiutil detach $(WC_DIR) -quiet -force
	rm -f "$(MASTER_DMG)"
	hdiutil resize -sectors min "$(WC_DMG)" -shrinkonly -nofinalgap
	hdiutil convert "$(WC_DMG)" -quiet -format UDZO -imagekey zlib-level=9 -o "$(MASTER_DMG)"
	rm -rf $(WC_DIR)
	@echo

.PHONY: clean
clean:
	-rm -rf $(TEMPLATE_DMG) $(MASTER_DMG) $(WC_DMG)
