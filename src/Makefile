# This file is part of RetCon: Retro Console Controller.
#
# RetCon is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# RetCon is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with RetCon.  If not, see <http://www.gnu.org/licenses/>.

CXXFLAGS += -std=c++11 -O3 -Wall
CFLAGS += -O3 -Wall
LDFLAGS =

all: retcon sixpair

%.o: %.cc
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

retcon: retcon.o
	$(CXX) $< -o $@ $(LDFLAGS) -lpigpio -lpthread

sixpair: sixpair.o
	$(CXX) $< -o $@ $(LDFLAGS) -lusb

install: all
	install -o root -m 0755 -d $(DESTDIR)/usr/sbin
	install -o root -m 0755 retcon $(DESTDIR)/usr/sbin/retcon
	install -o root -m 0755 sixpair $(DESTDIR)/usr/sbin/sixpair
	install -o root -m 0755 -d $(DESTDIR)/lib/udev/rules.d
	install -o root -m 0644 sixpair.rules $(DESTDIR)/lib/udev/rules.d/66-sixpair.rules

clean:
	rm -f retcon sixpair *.o
