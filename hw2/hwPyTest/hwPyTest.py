import sys
import numpy as np
import matplotlib.pyplot as plot

sys.path.append('../Debug')
import hw2py

# Setup Varibles
a = np.asarray([-0.2326, 0.3249], dtype=np.float32);
b = np.asarray([0.3375, 0, -0.3375], dtype=np.float32);
x = np.ones(15, dtype=np.float32)
i = np.zeros(15, dtype=np.float32)
i[0] = 1

# Test Impulse Response (Part C)
r = hw2py.filter(b, a, i)
plot.plot(r)
plot.title('Filter: Impluse Response (Part C)')
plot.show()

# Test Step Response (Part D)
y = hw2py.filter(b, a, x)
plot.plot(y)
plot.title('Filter: Step Response (Part D)')
plot.show()

# Test Convolution (Part E)
c = hw2py.conv(x, r[0:5])
plot.plot(c)
plot.title('Conv: Step Response')
plot.show()

e = c - y
plot.plot(e)
plot.title('Conv: Step Response Error')
plot.show()
