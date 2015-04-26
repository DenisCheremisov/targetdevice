include $(TOPDIR)/rules.mk

PKG_NAME:=tdevice
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/tdevice-$(PKG_VERSION)
PKG_SOURCE:=tdevice-$(PKG_VERSION).tar.gz
PKG_CAT:=zcat
PKG_BUILD_DEPENDS:=libyaml libbsd

include $(INCLUDE_DIR)/package.mk

define Package/tdevice
	SECTION:=utils
	CATEGORY:=Network
	DEFAULT:=y
	TITLE:=Target device controller
	DEPENDS:=libyaml libbsd
endef

define Package/tdevice/description
	Target device remote control
endef

define Build/Configure
  $(call Build/Configure/Default,--with-linux-headers=$(LINUX_DIR))
endef

define Package/tdevice/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/tdevice/tdevice $(1)/usr/bin/
endef

$(eval $(call BuildPackage,tdevice))
