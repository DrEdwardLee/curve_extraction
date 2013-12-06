% Wrapper for mex function.
function cost = curve_info(unary, path, settings)

% Check file modification dates and recompile mex file
my_name = mfilename('fullpath');
[base_path, base_name, ~] = fileparts(my_name);
compile(base_path, base_name)

if (size(path,2) == 2)
   path = [path ones(size(path,1),1)]; 
end

[cost.total, cost.unary, cost.length, cost.curvature, cost.torsion] ...
	= curve_info_mex(unary, path, settings);
