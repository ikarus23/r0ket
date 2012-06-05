#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
#  altitude.py
#
#  Usage: python3 altitude.py path/to/BPM085/logfile X
#   X = display the average over X log entrys.
#  
#  Copyright 2012 IKARUS <ikarus@earth>
#  
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#  
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#  
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
#  MA 02110-1301, USA.
#  
#  

import sys, math

def main():
    fobj = open(sys.argv[1], "r")
    i = 0
    value = 0
    for line in fobj:
        value += int(line.split(",")[1])
        i = i + 1
        if i % int(sys.argv[2]) == 0:
            value = value / int(sys.argv[2])
            i = 0
            print("%.1fM" % calcAltitude(value))
            value = 0
    return 0

def calcAltitude(pressure):
    A = float(pressure)/101325
    B = 1/5.25588
    C = math.pow(A,B)
    C = 1 - C
    C = C /0.0000225577
    return C

if __name__ == '__main__':
    main()

