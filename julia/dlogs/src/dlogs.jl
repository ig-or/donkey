module dlogs

using xqdata
using QWTWPlot
using Statistics
using Printf
using LinearAlgebra
#using Libdl
function __init__()
	if Sys.iswindows()
		startXqLoader(0, libFile = raw"C:\Users\ISandler\space\donkey\lib-vs-x64\release\xqloader.dll");
	else
		startXqLoader(0, libFile = "/home/igor/space/teetest/lib/release/libxqloader.so")
	end
	qstart()
	qsmw()
end

function drawc100(file)
	c100 = xqget(file, 1000.0);
	n = size(c100, 1)
	t = c100[:, 1]
	t1 = c100[:, 8]

	for i=1:n
		if abs(c100[i, 7] - 1.0) > 0.001
			c100[i, 6] = 0.0
		end
	end

	qfigure()
	qplot1(t, c100[:, 5], "minmax", "-eg", 4)
	qplot1(t, c100[:, 2], "a", "-eb", 3)
	qplot1(t, c100[:, 3], "aa", "-ek", 3)
	qplot1(t, c100[:, 4], "bb", "-em", 3)

	qxlabel("seconds")
	qtitle("motor control")

	qfigure()
	qplot1(t, c100[:, 6], "us_distance_mm1", "-ec", 5)
	qplot1(t, c100[:, 9], "us_distance_mm2", "-eb", 3)
	qtitle("distance fron the sensor");

	qfigure()
	qplot1(t, c100[:, 12], "mcState", "-ey", 7)
	qplot1(t, c100[:, 7], "quality1", "-ec", 5)
	qplot1(t, c100[:, 10], "quality2", "-eb", 3)
	qtitle("quality");

end

export drawc100


end # module dlogs
