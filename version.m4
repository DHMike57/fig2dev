#
# This file is part of Fig2dev: Translate fig code to various devices.
#
#	version.m4: Version information, included by configure.ac.
#	Author: Thomas Loimer <thomas.loimer@tuwien.ac.at>, 2016
#

dnl The version information is kept separately from configure.ac.
dnl Thus, configure.ac can remain unchanged between different versions.
dnl The values in this file are set by update_version_m4 if
dnl ./configure is called with --enable_versioning.

m4_define([FIG_VERSION], [3.2.6a])

dnl AC_INIT does not have access to shell variables.
dnl Therefore, define RELEASEDATE as a macro.
m4_define([RELEASEDATE], [Jan 2017])
