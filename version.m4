dnl version.m4 - Version information, included by configure.ac.
dnl This file is part of fig2dev - Translate Fig code to various devices.
dnl
dnl The version information is kept separately from configure.ac, because
dnl version.m4 is modified frequently from the toplevel Makefile, using
dnl sed, git describe and date. Thus, configure.ac can remain unchanged.

m4_define([FIG_VERSION], [3.2.6-rc])

dnl m4_define must be called before AC_INIT - but, I believe,
dnl shell-variables can only be defined after a call to AC_INIT.
dnl Therefore, define RELEASE_DATE as a macro.
m4_define([RELEASEDATE], [Jul 2016])
