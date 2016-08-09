# version.m4 - Version information, included by configure.ac.
# This file is part of fig2dev - Translate Fig code to various devices.
dnl
dnl The version information is kept separately from configure.ac, because
dnl version.m4 is modified frequently from the toplevel Makefile, using
dnl sed, git describe and date. Thus, configure.ac can remain unchanged.

m4_define([FIG_VERSION], [3.2.6])

dnl AC_INIT does not have access to shell variables.
dnl Therefore, define RELEASEDATE as a macro.
m4_define([RELEASEDATE], [Aug 2016])
