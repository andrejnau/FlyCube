#pragma once

#include <utility>


#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

using line = std::pair<glm::vec2, glm::vec2>;

static float dist(glm::vec2 &a, glm::vec2 &b)
{
	return sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

static double getF(int n)
{
    double res = 1;
    for (int i = 2; i <= n; ++i)
    {
        res *= i;
    }
    return res;
}

static double getC(int k, int n)
{
    return getF(n) / (getF(k)  * getF(n - k));
}

static std::vector<std::vector<double> > getHb(int n)
{
	std::vector<std::vector<double> > h(n, std::vector<double>(n));
    for (int i = 0; i < n; ++i)
    {
        for (int j = i; j < n; ++j)
        {
            h[i][j] = getC(j, n-1) * getC(i, j);
            if ((j - i) % 2 != 0)
                h[i][j] = -h[i][j];
        }
    }
	printf("getHb\n");
	for (int i = 0; i < n; ++i)
	{
		for (int j = 0; j < n; ++j)
		{
			printf("%4f ", h[i][j]);
		}
		printf("\n");
	}
    return std::move(h);
}

static std::vector<std::vector<double> > getHi(int n)
{
	std::vector<std::vector<double> > h(n, std::vector<double>(n, 1));
	std::vector<double> b(n);
    for (int i = 0; i < n; ++i)
    {
        b[i] = i;
    }
    for (int i = 1; i < n; ++i)
    {
        for (int j = 0; j < n; ++j)
        {
            h[i][j] = h[i - 1][j] * b[j];
        }
    }
	printf("getHi\n");
	for (int i = 0; i < n; ++i)
	{
		for (int j = 0; j < n; ++j)
		{
			printf("%4f ", h[i][j]);
		}
		printf("\n");
	}

    return std::move(h);
}

static std::vector<std::vector<double> > getD(int n)
{
	std::vector<std::vector<double> > h(n, std::vector<double>(n, 0));
    double pow = 1;
    for (int i = 0; i < n; ++i)
    {
        h[i][i] = pow;
        pow *= n - 1;
    }
	printf("getD\n");
	for (int i = 0; i < n; ++i)
	{
		for (int j = 0; j < n; ++j)
		{
			printf("%4f ", h[i][j]);
		}
		printf("\n");
	}
    return std::move(h);
}

static std::vector<std::vector<double> > getInverseMatrix(const std::vector<std::vector<double> > vec)
{
	std::vector<std::vector<double> > v = vec;
    int n = v.size();
	std::vector<std::vector<double> > e(n, std::vector<double>(n));
    for (int i = 0; i < n; ++i)
    {
        e[i][i] = 1;
    }
    double eps = 1e-12;
    for (int k = 0; k < n; ++k)
    {
        double t0 = v[k][k];
        if (fabs(t0) < eps)
        {
            int q = -1;
            for (int i = k + 1; i < n; ++i)
            {
                if (fabs(v[i][k]) > eps)
                {
                    q = i;
                    break;
                }
            }
            if (q != -1)
            {
                for (int i = k; i < n; ++i)
                {
					std::swap(v[q][i], v[k][i]);
					std::swap(e[q][i], e[k][i]);
                }
                t0 = v[k][k];
            }
        }

        assert(!(fabs(t0) < eps));

        for (int j = 0; j < n; ++j)
        {
            v[k][j] /= t0;
            e[k][j] /= t0;
        }

        for (int i = k + 1; i < n; ++i)
        {
            double t = v[i][k];
            for (int j = 0; j < n; ++j)
            {
                v[i][j] -= v[k][j] * t;
                e[i][j] -= e[k][j] * t;
            }
        }
    }
    for (int k = n - 1; k >= 0; --k)
    {
        for (int i = k - 1; i >= 0; --i)
        {
            double t = v[i][k];
            for (int j = 0; j < n; ++j)
            {
                v[i][j] -= v[k][j] * t;
                e[i][j] -= e[k][j] * t;
            }
        }
    }

    return std::move(e);
}

static double binpow(double a, int n) 
{
	double res = 1;
	bool f = false;
	if (n < 0)
	{
		n = -n;
		f = true;
	}
	while (n) 
	{
		if (n & 1)
			res *= a;
		a *= a;
		n >>= 1;
	}
	if (f)
		return 1.0 / res;
	else
		return res;
}

static std::vector<std::vector<double> > operator*(const std::vector<std::vector<double> > & a, const std::vector<std::vector<double> > & b)
{
    int n = a.size();
	int m = b[0].size();
	assert(a[0].size() == b.size());
	std::vector<std::vector<double> > c(n, std::vector<double>(m));
    for (int i = 0; i < (int)c.size(); ++i)
    {
        for (int j = 0; j < (int)c[i].size(); ++j)
        {
            c[i][j] = 0;
            for (int k = 0; k < (int)b.size(); ++k)
            {
                c[i][j] += a[i][k] * b[k][j];
            }
        }
    }
    return std::move(c);
}
