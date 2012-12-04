#
# This file is part of axis_profile
#
# Copyright (C) 2004 Axis Communications AB, LUND, SWEDEN
#
# axis_profile is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# axis_profile is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with axis_profile.  If not, see <http://www.gnu.org/licenses/>.
#


AXIS_USABLE_LIBS = UCLIBC GLIBC
AXIS_AUTO_DEPEND = yes
-include $(AXIS_TOP_DIR)/tools/build/rules/common.mak

CFLAGS    += -DAXIS_PROFILER_VERSION=\"3.3.2\"

PROGS     = axis_profile
SCRIPTS   = $(wildcard *.exp)

prefix    ?= /usr/local
build_prefix ?= $(prefix)
BINDIR    ?= $(build_prefix)/bin
INSTALL   ?= install

SRCS      = axis_profile.c application.c file.c function.c map.c

OBJS      = $(SRCS:.c=.o)

##########################################################################

all:	$(PROGS)

$(PROGS):	$(OBJS)

install:	all
	$(INSTALL) -d $(BINDIR)
	$(INSTALL) -m 0755 $(PROGS) $(SCRIPTS) $(BINDIR)

clean:
	$(RM) $(PROGS) *.o core *~
