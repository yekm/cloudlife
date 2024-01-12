#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace IDL {
namespace CB_YIGnBu {

float colormap_red(float x) {
	if (x < 0.2523055374622345) {
		return (-5.80630393656902E+02 * x - 8.20261301968494E+01) * x + 2.53829637096771E+02;
	} else if (x < 0.6267540156841278) {
		return (((-4.07958939010649E+03 * x + 8.13296992114899E+03) * x - 5.30725139102868E+03) * x + 8.58474724851723E+02) * x + 2.03329669375107E+02;
	} else if (x < 0.8763731146612115) {
		return 3.28717357910916E+01 * x + 8.82117255504255E+00;
	} else {
		return -2.29186583577707E+02 * x + 2.38482038123159E+02;
	}
}

float colormap_green(float x) {
	if (x < 0.4578040540218353) {
		return ((4.49001704856054E+02 * x - 5.56217473429394E+02) * x + 2.09812296466262E+01) * x + 2.52987561849833E+02;
	} else {
		return ((1.28031059709139E+03 * x - 2.71007279113343E+03) * x + 1.52699334501816E+03) * x - 6.48190622715140E+01;
	}
}

float colormap_blue(float x) {
	if (x < 0.1239372193813324) {
		return (1.10092779856059E+02 * x - 3.41564374557536E+02) * x + 2.17553885630496E+02;
	} else if (x < 0.7535201013088226) {
		return ((((3.86204601547122E+03 * x - 8.79126469446648E+03) * x + 6.80922226393264E+03) * x - 2.24007302003438E+03) * x + 3.51344388740066E+02) * x + 1.56774650431396E+02;
	} else {
		return (((((-7.46693234167480E+06 * x + 3.93327773566702E+07) * x - 8.61050867447971E+07) * x + 1.00269040461745E+08) * x - 6.55080846112976E+07) * x + 2.27664953009389E+07) * x - 3.28811994253461E+06;
	}
}

float4 colormap(float x) {
	float r = clamp(colormap_red(x) / 255.0, 0.0, 1.0);
	float g = clamp(colormap_green(x) / 255.0, 0.0, 1.0);
	float b = clamp(colormap_blue(x) / 255.0, 0.0, 1.0);
	return float4(r, g, b, 1.0);
}

} // namespace CB_YIGnBu
} // namespace IDL
} // namespace colormap
