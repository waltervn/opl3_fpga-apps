#
# This file is the midi recipe.
#

SUMMARY = "OPL3_FPGA MIDI Device Initialization"
SECTION = "PETALINUX/apps"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://midi \
	"
S = "${WORKDIR}"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

inherit update-rc.d

INITSCRIPT_NAME = "midi"
INITSCRIPT_PARAMS = "start 99 S ."

do_install() {
             install -d ${D}${sysconfdir}/init.d
             install -m 0755 ${S}/midi ${D}${sysconfdir}/init.d/midi
}

FILES_${PN} += "${sysconfdir}/*"
