#!/usr/bin/env python
# -*- coding: utf-8 -*-
import sys
import numpy as np

def load_matrix(filename):
    file = open(filename, "r")
    arr = []
    for line in file.readlines():
        line = line.strip()
        numbers = [int(i) for i in line.split()]
        arr.append(numbers)
    return np.array(arr)


first, second, third = sys.argv[1:]
A = load_matrix(first) 
B = load_matrix(second) 
C = load_matrix(third) 
print(A.shape, B.shape)

D = A.dot(B)
print((C == D).all())
