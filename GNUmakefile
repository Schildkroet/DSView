##
## This file is part of the DSView project.
## DSView is based on PulseView.
##
## Copyright (C) 2026 Schildkroet
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
##

##
## Convenience wrapper so a bare `make` in the source root works, while
## every generated file stays in ./build (the source tree is never touched).
##
##   make             configure ./build if needed, then build (in parallel)
##   make install     build if needed, then install (`sudo make install`)
##   make uninstall   remove what `make install` installed (`sudo make uninstall`)
##   make purge       uninstall, and delete the DSView settings too
##   make deb         build a .deb package in ./build (no sudo needed), then
##                    install it with `sudo apt install ./build/dsview_*.deb`
##   make clean       remove compiled objects, keep the configuration
##   make distclean   remove ./build entirely
##
##   make JOBS=8                      override the job count
##   make CMAKE_ARGS="-G Ninja"       extra arguments for the first configure
##
## Under sudo, configuring and compiling run as the invoking user ($SUDO_USER);
## only the copy into the system runs as root, and the install manifest it
## writes is handed back afterwards. So `sudo make install` never leaves
## root-owned files in ./build.
##

BUILD_DIR  ?= build
BUILD_TYPE ?= Release
CMAKE_ARGS ?=

# Leave two cores to the rest of the system, but never drop below one job.
# getconf is POSIX; sysctl covers macOS, and the trailing 1 covers both failing.
JOBS ?= $(shell \
	cores=$$(getconf _NPROCESSORS_ONLN 2>/dev/null \
		|| sysctl -n hw.ncpu 2>/dev/null \
		|| echo 1); \
	jobs=$$((cores - 2)); \
	[ "$$jobs" -ge 1 ] 2>/dev/null || jobs=1; \
	echo $$jobs)

# cmake drives its own (possibly make-based) build; don't leak our flags into it.
SUBCMAKE = env -u MAKEFLAGS -u MAKELEVEL -u MFLAGS cmake

# Root via sudo: drop back to the calling user for everything but the install.
AS_USER :=
ifeq ($(shell id -u),0)
ifneq ($(filter-out root,$(SUDO_USER)),)
AS_USER := sudo -u $(SUDO_USER)
endif
endif

.PHONY: all build configure install uninstall purge deb clean distclean

all: build

$(BUILD_DIR)/CMakeCache.txt:
	$(AS_USER) $(SUBCMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(CMAKE_ARGS)

configure: $(BUILD_DIR)/CMakeCache.txt

build: configure
	$(AS_USER) $(SUBCMAKE) --build $(BUILD_DIR) --parallel $(JOBS)

install: build
	$(SUBCMAKE) --install $(BUILD_DIR)
ifneq ($(AS_USER),)
	chown $(SUDO_UID):$(SUDO_GID) $(BUILD_DIR)/install_manifest*.txt
endif

# Removes the files listed in $(BUILD_DIR)/install_manifest.txt, the decoder
# caches and the then empty DSView folders; keeps anything it didn't install.
# Compiles nothing. For an install made from another build folder, name its
# manifest:   sudo make uninstall MANIFEST=../old-tree/build/install_manifest.txt
# See CMake/cmake_uninstall.cmake.in.
UNINSTALL = $(SUBCMAKE) $(if $(MANIFEST),-DMANIFEST="$(abspath $(MANIFEST))")

# Only configures, for a build folder from before the uninstall script existed.
$(BUILD_DIR)/cmake_uninstall.cmake:
	$(AS_USER) $(SUBCMAKE) -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) $(CMAKE_ARGS) > /dev/null

uninstall: $(BUILD_DIR)/cmake_uninstall.cmake
	$(UNINSTALL) -P $<

# Also deletes the settings and device profiles of whoever runs it, of the user
# behind sudo, and root's (there if DSView was ever started with sudo). Linux
# locations; other DreamSourceLab programs' files there are left alone.
# Close DSView first: it writes its settings when it exits.
PURGE_HOMES := $(sort $(HOME) \
	$(if $(filter-out root,$(SUDO_USER)),$(shell getent passwd $(SUDO_USER) | cut -d: -f6)))

purge: $(BUILD_DIR)/cmake_uninstall.cmake
	$(UNINSTALL) -DPURGE=ON -P $<
	@for h in $(PURGE_HOMES); do \
		for f in "$$h/.config/DreamSourceLab/DSView.conf" \
		         "$$h/.config/DreamSourceLab/DSView.conf.lock" \
		         "$$h/.local/share/DreamSourceLab/DSView"; do \
			if [ -e "$$f" ]; then rm -rf "$$f" && echo "-- Removed settings: $$f"; fi; \
		done; \
		rmdir "$$h/.config/DreamSourceLab" "$$h/.local/share/DreamSourceLab" 2>/dev/null || true; \
	done
	@echo "-- DSView and its settings are removed."

# Builds $(BUILD_DIR)/dsview_<version>_<arch>.deb, installing under /usr; see
# the CPack settings in CMakeLists.txt. apt installs it with the libraries it
# needs. A `make install` copy in /usr/local would be started instead of it, as
# /usr/local/bin comes first in PATH: hence the note.
deb: build
	@command -v dpkg-shlibdeps >/dev/null 2>&1 || { \
		echo "make deb needs dpkg-shlibdeps, from dpkg-dev: sudo apt install dpkg-dev"; exit 1; }
	cd $(BUILD_DIR) && $(AS_USER) cpack -G DEB
	@deb=$$(ls -t $(BUILD_DIR)/dsview_*.deb | head -1); \
	case "$$deb" in /*) ;; *) deb="./$$deb" ;; esac; \
	echo "-- Package: $$deb"; \
	echo "-- Install it with: sudo apt install $$deb"; \
	if [ -e /usr/local/bin/DSView ]; then \
		echo "-- Note: DSView from \`make install\` is in /usr/local, and would be started"; \
		echo "   instead of the package. Remove it first: sudo make uninstall"; \
	fi; \
	local_libs=$$(ldd $(BUILD_DIR)/bin/DSView 2>/dev/null | awk '$$3 ~ /^\/usr\/local\// {print "     " $$3}'); \
	if [ -n "$$local_libs" ]; then \
		echo "-- Warning: DSView uses these libraries from /usr/local, which no package"; \
		echo "   provides, so the .deb cannot depend on them. It runs here, but needs"; \
		echo "   them on any other machine:"; \
		echo "$$local_libs"; \
	fi

clean:
	@test ! -f $(BUILD_DIR)/CMakeCache.txt || $(AS_USER) $(SUBCMAKE) --build $(BUILD_DIR) --target clean

distclean:
	rm -rf $(BUILD_DIR)
