#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
#  altitude.py
#
#  Usage: python3 altitude.py <path/to/BPM085/logfile> <n>
#         n = display the average over n entries.
#  Example: python3 altitude.py BPM085.log 10
#
#  
#  Copyright 2012 ossi (and ikarus)
#

import sys
import math

def avg(list):
    """
    Returns the average over all elements of list.

    The argument 'list' has to have only numerical numbers.
    """
    return sum(list) / len(list)


def prepare_list(iterator, count):
    """
    Returns a list, witch has 'count' elements or an empty list.
    """
    prepared_list = list(iterator)
    if len(prepared_list) == count:
        return prepared_list
    return []


def get_values_from_file(file, count):
    """
    Generator witch gives 'count' values from the file

    'file' has to be a file object to a csv file. The second element
    has to be an integer.
    """
    for i in range(count):
        try:
            yield int(file.readline().strip().split(',')[1])
        except IndexError:
            # Ignore empty lines
            pass


def calc_altitude(list):
    """
    Calculates the altitude for all list elements and retruns them
    as list.

    The argument 'list' has to have only numerical numbers.
    """
    l = []
    for pressure in list:
        A = float(pressure)/101325
        B = 1/5.25588
        C = math.pow(A,B)
        C = 1 - C
        C = C /0.0000225577
        l.append(C)
    return l


def main(path, count):
    """
    Opens 'path' and converts the second column to altitude values
    and then builds the average over 'count' of those entries.
    Prints the output to stdout.
    """
    file = open(path)
    while True:
        try:
            print("%.2f" % avg(calc_altitude(prepare_list( \
                    get_values_from_file(file, count), count))))
        except ZeroDivisionError:
            break


if __name__ == '__main__':
    if len(sys.argv) != 3:
        print("Usage: python3 %s <path/to/BPM085/logfile> <n>\n"
        "       n = display the average over n entries" % sys.argv[0])
        exit(1)
    main(sys.argv[1], int(sys.argv[2]))

