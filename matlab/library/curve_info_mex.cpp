// This function both calculates the cost of a curve decomposed into
// data, length, curvature and torsion parts.
// It also return the length, curvature and torsion of the curve itself.
#include "curve_segmentation.h"

template<typename Data_cost, typename Length_cost, typename Curvature_cost, typename Torsion_cost>
void curve_info(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
	ASSERT(nlhs == 8);
	ASSERT(nrhs == 4 || nrhs == 5);

	// Parse data
	int curarg = 1;
	const matrix<double> data_matrix(prhs[curarg++]);
	const matrix<double> path(prhs[curarg++]);
  const matrix<int> connectivity(prhs[curarg++]);
	MexParams params(nrhs-curarg, prhs+curarg);
  InstanceSettings settings = parse_settings(params);


  // Returns length, curvature and data cost
  matrix<double> total_cost(1);

  matrix<double> total_data_cost(1);
  matrix<double> total_length_cost(1);
  matrix<double> total_curvature_cost(1);
  matrix<double> total_torsion_cost(1);
  

  matrix<double> curve_length(1);
  matrix<double> curve_curvature(1);
  matrix<double> curve_torsion(1);

  // Weighted by penalty function
  total_cost(0) = 0;
  total_data_cost(0) = 0;
  total_length_cost(0) = 0;
  total_curvature_cost(0) = 0;
  total_torsion_cost(0) = 0;

  // Length, curvature and torsion of curve
  curve_length(0) = 0;
  curve_curvature(0) = 0;
  curve_torsion(0) = 0;

  plhs[0] = total_cost;
  plhs[1] = total_data_cost;
  plhs[2] = total_length_cost;
  plhs[3] = total_curvature_cost;
  plhs[4] = total_torsion_cost;
  plhs[5] = curve_length;
  plhs[6] = curve_curvature;
  plhs[7] = curve_torsion;

  Data_cost data_cost(data_matrix, connectivity, settings.voxel_dimensions);

  // Data cost
  for (int k = 0; k < (int)path.M-1; k++)
  {
    total_data_cost(0) += data_cost( path(k+0,0) -1, path(k+0,1) -1, path(k+0,2) -1,
                                path(k+1,0) -1, path(k+1,1) -1, path(k+1,2) -1);
  }

  // Length
  Length_cost length_fun(data_matrix, settings.voxel_dimensions, 1);
  for (int k = 0; k < (int)path.M-1; k++)
  {
    curve_length(0) += length_fun(path(k+0,0) -1, path(k+0,1) -1, path(k+0,2) -1,
                                  path(k+1,0) -1, path(k+1,1) -1, path(k+1,2) -1);
  }


 	// Curvature
  Curvature_cost curvature_fun(data_matrix, settings.voxel_dimensions, 1, settings.curvature_power);
 	for (int k = 0; k < (int)path.M-2; k++)
 	{
     curve_curvature(0) +=
     curvature_fun((path(k+0,0)-1),
           				      (path(k+0,1)-1),
           				      (path(k+0,2)-1),
           				      (path(k+1,0)-1),
           				      (path(k+1,1)-1),
           				      (path(k+1,2)-1),
            			      (path(k+2,0)-1),
            			      (path(k+2,1)-1),
            			      (path(k+2,2)-1));
 	}

  // Torsion
  Torsion_cost torsion_fun(data_matrix, settings.voxel_dimensions, 1, settings.torsion_power);
  for (int k = 0; k < (int)path.M-3; k++)
  {
    curve_torsion(0) +=
    torsion_fun((path(k+0,0)-1),
                     (path(k+0,1)-1),
                     (path(k+0,2)-1),
                     (path(k+1,0)-1),
                     (path(k+1,1)-1),
                     (path(k+1,2)-1),
                     (path(k+2,0)-1),
                     (path(k+2,1)-1),
                     (path(k+2,2)-1),
                     (path(k+3,0)-1),
                     (path(k+3,1)-1),
                     (path(k+3,2)-1));
  }

  // Add weights
  // The if statement is added in order to avoid potential
  // numerical issues in the curvature/torsion calculations.
  if (settings.length_penalty > 0)
    total_length_cost(0) = curve_length(0)*settings.length_penalty;
  else
    total_length_cost(0) = 0;

  if (settings.curvature_penalty > 0)
    total_curvature_cost(0) = curve_curvature(0)*settings.curvature_penalty;
  else
    total_curvature_cost(0) = 0;

  if (settings.torsion_penalty > 0)
    total_torsion_cost(0) = curve_torsion(0)*settings.torsion_penalty;
  else
    total_torsion_cost(0) = 0;

  total_cost(0) = total_data_cost(0) + total_length_cost(0) + total_curvature_cost(0) + total_torsion_cost(0);
}

void mexFunction(int            nlhs,     /* number of expected outputs */
                 mxArray        *plhs[],  /* mxArray output pointer array */
                 int            nrhs,     /* number of inputs */
                 const mxArray  *prhs[]   /* mxArray input pointer array */)
{
 int buff_size = 1024;
 char problem_type[buff_size];
 if (mxGetString(prhs[0], problem_type, buff_size)) 
   throw runtime_error("First argument must be a string.");

  if (!strcmp(problem_type,"linear_interpolation"))
    curve_info<Linear_data_cost, Euclidean_length, Euclidean_curvature, Euclidean_torsion>(nlhs, plhs, nrhs, prhs);
  else if (!strcmp(problem_type,"edge"))
    curve_info<Edge_data_cost, Euclidean_length, Euclidean_curvature, Euclidean_torsion>(nlhs, plhs, nrhs, prhs);
  else if (!strcmp(problem_type,"geodesic"))
    curve_info<Zero_data_cost, Geodesic_length, Geodesic_curvature, Zero_torsion>(nlhs, plhs, nrhs, prhs);
  else
    throw runtime_error("Unknown data type");
}