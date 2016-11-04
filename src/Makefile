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

CXXFLAGS = -std=c++11 -O3 -Wall
LDFLAGS =

all: controller sixpair

%.o: %.cc
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

controller: controller.o
	$(CXX) $< -o $@ $(LDFLAGS) -lpigpio -lrt -lpthread

sixpair: sixpair.o
	$(CXX) $< -o $@ $(LDFLAGS) -lusb

clean:
	rm -f controller sixpair *.o
