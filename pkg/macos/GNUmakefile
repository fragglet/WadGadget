
TOPLEVEL = ../..
APP_DIR = staging/WadGadget.app
APP_TOP_DIR = $(APP_DIR)/Contents
APP_BIN_DIR = $(APP_DIR)/Contents/MacOS
APP_RES_DIR = $(APP_DIR)/Contents/Resources
DOC_FILES = README.md COPYING.md
DMG = wadgadget.dmg
ICONS = app.icns wadfile.icns
PANDOC_FLAGS = --embed-resources

$(DMG) : tmp.dmg
	rm -f $@
	hdiutil convert -format UDBZ -o $@ tmp.dmg
	rm -f tmp.dmg

tmp.dmg : staging
	rm -f $@
	hdiutil makehybrid -hfs -hfs-volume-name "WadGadget"     \
	        -hfs-openfolder staging staging -o tmp.dmg

staging: $(ICONS) main
	rm -rf staging
	mkdir staging

	mkdir -p "$(APP_BIN_DIR)" "$(APP_RES_DIR)"
	cp Resources/Info.plist Resources/PkgInfo "$(APP_TOP_DIR)"
	cp $(ICONS) "$(APP_RES_DIR)"
	cp main "$(APP_BIN_DIR)"
	./cp-with-libs $(TOPLEVEL)/src/wadgadget "$(APP_BIN_DIR)"

	for d in $(DOC_FILES); do \
		pandoc $(PANDOC_FLAGS) -f gfm \
		    -o staging/$$(basename $$d .md).html \
		    $(TOPLEVEL)/$$d; \
	done

%.iconset: Resources/%.png
	mkdir -p $@
	sips -z 64 64 $< --out $@/icon_32x32@2x.png
	sips -z 256 256 $< --out $@/icon_128x128@2x.png

%.icns: %.iconset
	iconutil -c icns $<

main: main.swift
	swiftc $< -o $@

clean:
	rm -f $(DMG) $(ICONS) tmp.dmg
	rm -rf staging
	rm -f main
	rm -rf *.iconset

.PHONY: clean
