#
# Copyright (C) 2010 Denis Cheremisov <denis@cheremisov.net>
#
# This is free software, licensed under the GNU General Public License v3.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=tdevice
PKG_RELEASE:=1

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)
PKG_BUILD_DEPENDS := +libpthread +libyaml +libopenssl +libstdc++

include $(INCLUDE_DIR)/package.mk

define Package/tdevice
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Target device remote control actor
  MAINTAINER:=Denis Cheremisov <denis@cheremisov.net>
  DEPENDS:=+libpthread +libyaml +libopenssl +libstdcpp
endef

define Package/tdevice/description
  This software implements remote control over target device via periodically getting
  instructions from remote control sources.
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Package/tdevice/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tdevice $(1)/usr/bin/tdevice
endef

$(eval $(call BuildPackage,tdevice,+libopenssl,+libyaml,+libstdc++))
