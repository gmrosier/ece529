#define BOOST_PYTHON_STATIC_LIB
#include <boost/python/numpy.hpp>
#include "hw2.h"

namespace p = boost::python;
namespace np = boost::python::numpy;

np::ndarray np_filter(np::ndarray b, np::ndarray a, np::ndarray x)
{
	np::dtype dtype = np::dtype::get_builtin<float>();
	Py_intptr_t const * shape = x.get_shape();
	int nd = x.get_nd();

	if (np::equivalent(x.get_dtype(), dtype))
	{
		int l = (int)x.shape(0);
		int m = (int)b.shape(0);
		int n = (int)a.shape(0);

		float * bc = reinterpret_cast<float *>(b.get_data());
		float * ac = reinterpret_cast<float *>(a.get_data());
		float * xc = reinterpret_cast<float *>(x.get_data());

		np::ndarray y = np::zeros(nd, shape, dtype);
		float * yc = reinterpret_cast<float *>(y.get_data());
		
		filter(bc, m, ac, n, xc, l, yc);
		return y;
	}
	else
	{

		return np::zeros(nd, shape, x.get_dtype());
	}
	
}


np::ndarray np_conv(np::ndarray x, np::ndarray h)
{
	np::dtype dtype = np::dtype::get_builtin<float>();
	Py_intptr_t const * shape = x.get_shape();
	int nd = x.get_nd();

	if (np::equivalent(x.get_dtype(), dtype))
	{
		int l = (int)x.shape(0);
		int m = (int)h.shape(0);

		float * xc = reinterpret_cast<float *>(x.get_data());
		float * hc = reinterpret_cast<float *>(h.get_data());
		

		np::ndarray y = np::zeros(nd, shape, dtype);
		float * yc = reinterpret_cast<float *>(y.get_data());

		conv(xc, l, hc, m, yc);
		return y;
	}
	else
	{
		return np::zeros(nd, shape, x.get_dtype());
	}

}

BOOST_PYTHON_MODULE(hw2py)
{
	np::initialize();
	p::def("filter", np_filter);
	p::def("conv", np_conv);
}
