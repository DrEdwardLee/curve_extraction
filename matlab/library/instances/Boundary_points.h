#pragma once
#include <spii-thirdparty/fadiff.h>
#include <spii/auto_diff_term.h>
namespace fadbad
{

template<unsigned n>
F<double, n> abs(F<double, n> x)
{
	if (spii::to_double(x) < 0) {
		return -x;
	}
	else {
		return x;
	}
}

template<unsigned n>
F<F<double, n>, n> abs(F<F<double, n>, n> x)
{
	if (spii::to_double(x) < 0) {
		return -x;
	}
	else {
		return x;
	}
}
}

// Container
template<typename R>
class Boundary_points
{
public:
	Boundary_points(const matrix<double>& data, const vector<double>& vd) 
	: data(data), vd(vd)
	{};

	int num_pairs()
	{
		return v_d11.size();
	}

	// First point
	void add(R _x, R _y)
	{
		v_x.push_back(_x);
		v_y.push_back(_y);

	}

	typedef std::tuple<R,R,R,R,R,R, double, double, double> pair_tuple;

	pair_tuple get_pair(int pair)
	{
	  R x0 = v_x[pair];
    R x1 = v_x[pair+1];

  	R y0 = v_y[pair];
  	R y1 = v_y[pair+1];

    R dx = (x0-x1)*vd[0];
    R dy = (y0-y1)*vd[1];

    // Fractional part
    x1 = (x1-std::floor( spii::to_double(x0) ))*vd[0];
    y1 = (y1-std::floor( spii::to_double(y0) ))*vd[1];

    x0 = (x0-std::floor( spii::to_double(x0) ))*vd[0];
    y0 = (y0-std::floor( spii::to_double(y0) ))*vd[1];

    return pair_tuple(x0,y0,x1,y1,dx,dy, v_d11[pair], v_d10[pair], v_d01[pair]);
	}

	// All subsequent points
	void add(R _x, R _y, int linear_index) 
	{
		int y0 = linear_index/data.M;
		int x0 = linear_index - y0*data.M;

		double i00 = data_value(x0 + 0, y0 + 0)*vd[2];
		double i01 = data_value(x0 + 0, y0 + 1)*vd[2];
		double i10 = data_value(x0 + 1, y0 + 0)*vd[2];
		double i11 = data_value(x0 + 1, y0 + 1)*vd[2];

		v_d11.push_back( (i00 + i11 - i10- i01)/(vd[0]*vd[1]) );
		v_d10.push_back( (i10 - i00)/(vd[0]*vd[1]) );
		v_d01.push_back( (i01 - i00)/(vd[0]*vd[1]) );

		v_x.push_back(_x);
		v_y.push_back(_y);
  }

  // Sample data or closest point inside data.
  double data_value(int x, int y) const
  {
    x = feasible(x, data.M);
    y = feasible(y, data.N);

    return data(x,y);
  }

  int feasible(int x, int M) const
  {
    if (x < 0)
      return 0;

    if (x > M-1)
      return M-1;

    return x;
  }

protected:
	std::vector<R> v_x;
	std::vector<R> v_y;

	std::vector<double> v_d11;
	std::vector<double> v_d01;
	std::vector<double> v_d10;

	const matrix<double> data;
	const vector<double> vd;
};

class Boundary_points_calculator
{
	public:
	  Boundary_points_calculator(const matrix<double>& data, const vector<double>& vd)
	    :  data(data), vd(vd), M(data.M), N(data.N)
	 {}

	// This is similar to evaluate_line_integral
	template<typename R>
	Boundary_points<R> operator () (R sx, R sy, R ex, R ey) const
	{
		using std::abs;
		using std::sqrt;

		R dx = (ex - sx)*vd[0];
		R dy = (ey - sy)*vd[1];
		R line_length = sqrt(dx*dx + dy*dy);

		typedef std::pair<R, int> crossing;

		// Invariant of direction
		auto intersection =
		[&]
		(int index_change, R line_length, R dx, R start,
		 std::vector<crossing>& crossings) -> void
		{
			R k = dx/line_length;

			if (abs(spii::to_double(k)) <= 1e-4f)
				return;

			R current;
			if (k > 0)
			{
				current = 1.0 - fractional_part(start);
			}
			else
			{
				current = fractional_part(start);
				index_change = -index_change;
			}

			if (current < 1e-6)
				current = current +1;

			for (; current <= abs(dx) - 1e-6; current = current +1)
				crossings.push_back( crossing( abs(current/dx) , index_change) );
		};

		std::vector<crossing>* scratch_space;
		#ifdef USE_OPENMP
			// Maximum 100 threads.
			// Support for C++11 thread_local is not very good yet.
			static std::vector<crossing> local_space[100];
			scratch_space = &local_space[omp_get_thread_num()];
		#else
			static std::vector<crossing> local_space;
			scratch_space = &local_space;
		#endif

		int source_id =  int( spii::to_double(sx) )  + int( spii::to_double(sy) )*M;
		if ((dx < 0) && (fractional_part(dx) < 1e-6))
			source_id -= 1;

		if ((dy < 0) && (fractional_part(dy) < 1e-6))
			source_id -= M;

		scratch_space->clear();
		scratch_space->push_back( crossing(0, 0) );
		intersection(1,line_length, dx, sx, *scratch_space);
		intersection(M,line_length, dy, sy, *scratch_space);
		std::sort(scratch_space->begin(), scratch_space->end());

		Boundary_points<R> points(data, vd);
		points.add(sx,sy);

		// Remove small increments
		auto prev = scratch_space->begin();
		auto next = scratch_space->begin();
		next++;
		for (; next != scratch_space->end(); prev++, next++)
		{
			if (abs(next->first - prev->first) > 1e-6)
			{				
				R xc = sx + (next->first)*dx;
				R yc = sy + (next->first)*dy;
				points.add(xc,yc, source_id);
			}

			source_id += next->second;
		}

		points.add(ex,ey, source_id);
		return points;
	}

	template<typename R> 
	R fractional_part(R x) const
	{
		double integer_part;
		modf(spii::to_double(x), &integer_part);
		return x - integer_part;
	}

  const matrix<double> data;
	const vector<double> vd;
	int M; // rows
	int N; // cols
};