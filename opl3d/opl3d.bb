#
# This file is the opl3d recipe.
#

SUMMARY = "OPL3_FPGA USB->OPL3 relay daemon"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://opl3d.c \
	   file://opl3d.sh \
	   file://Makefile \
		  "

S = "${WORKDIR}"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

do_compile() {
	     oe_runmake
}

inherit update-rc.d

INITSCRIPT_NAME = "opl3d.sh"
INITSCRIPT_PARAMS = "start 99 S ."

do_install() {
	     install -d ${D}${sysconfdir}/init.d
	     install -m 0755 ${S}/opl3d.sh ${D}${sysconfdir}/init.d/opl3d.sh
	     
         install -d ${D}${bindir}
         install -m 0755 ${S}/opl3d ${D}${bindir}	     
}
FILES_${PN} += "${sysconfdir}/*"
