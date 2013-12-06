% Wrapper for mex function.
function [optimized_path, info] = local_optimization(mesh_map, unary, input_path, settings)

% Check file modification dates and recompile mex file
my_name = mfilename('fullpath');
[base_path, base_name, ~] = fileparts(my_name);
compile(base_path, base_name)

% Checks for disallowed pixels
unary(mesh_map == 0) = max( max(unary(:))*1e2, 1e10);

if size(input_path, 2) == 2
   input_path = [input_path ones(size(input_path,1),1)]; 
end

[optimized_path, info] = local_optimization_mex(unary, input_path, settings);

% Check that the solution fulfills constraints
assert( size(optimized_path,2) == size(input_path, 2) );

% so = optimized_path(1:2,:);
% sp = input_path(1:2,:);
% assert(all( so(:) == sp(:) ));
% 
% eo = optimized_path(end:end-1,:);
% ep = input_path(end:end-1,:);
% assert(all( eo(:) == ep(:)  )); 
