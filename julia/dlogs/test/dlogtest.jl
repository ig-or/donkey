
ENV["PATH"] = raw"C:\programs\boost\178\lib64-msvc-14.3;" * ENV["PATH"]
#push!(LOAD_PATH, String(@__DIR__)) # need modules from the current folder
include("../src/dlogs.jl")

using .dlogs
using xqdata


#startXqLoader(0, libFile = raw"C:\Users\ISandler\space\donkey\lib-vs-x64\release\xqloader.dll");
file = raw"C:\Users\ISandler\space\donkey\lib-vs-x64\release\log_6.c100"

c100 = xqget(file, 1000.0);
size(c100)
n = size(c100, 1)
t = c100[:, 1]
t1 = c100[:, 8]


distance2 = [if c100[i, 7] ≈ 1.0 c100[:, 6]; else 0.0; end for i = 1:n]

dlogs.drawc100(file)