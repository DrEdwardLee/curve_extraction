% This script reproduces Figure 6 in
% Shortest Paths with Curvature and Torsion
% Petter Strandmark, Johannes Ul�n, Fredrik Kahl, Leo Grady. 
% International Conference on Computer Vision. 2013.
%
% Windows note:
% If you are using precompiled mex files
% you need Visual C++ Redistributable Packages for Visual Studio 2013:
% http://www.microsoft.com/en-us/download/details.aspx?id=40784
close all
clear all

% Regularizations
use_a_star = true;
curvature_regs = [0.25 50 500];
length_regs = [0.25 1 1000];

LAB = @(I) double(applycform(uint8(I), makecform('srgb2lab')));
RGB = @(I) double(applycform(uint8(I),makecform('lab2srgb')));

% Image
I = imread('../data/irrawaday-delta-sevcik.jpg');
I = I(40:end,:,:);
I = imresize(I,0.25);

% mesh_map defining allowed pixels
% Encoded as
% 0: Disallowed
% 1: Allowed
% 2: Start set
% 3: End set.
[w,h,c] = size(I);
mesh_map = ones([size(I,1) size(I,2)], 'int32');

% Start set
mesh_map(1:3, 1:round(end/2)) = 2;
mesh_map(end,1:round((1/20*h))) = 2;

% End set
mesh_map(end-3:end, :) = 3;

%% Unary, hand crafted
River_colors = {};
River_colors{end+1} = [169 151 137];
River_colors{end+1} = [200 156 119];
River_colors{end+1} = [255 255 255];
River_colors{end+1} = [191 194 202];
River_colors{end+1} = [233 225 233];
River_colors{end+1} = [131 116 83];
River_colors{end+1} = [182 149 121];
River_colors{end+1} = [205 176 165];
River_colors{end+1} = [156 133 102];

unary = inf(size(I,1), size(I,2));
target_intensity = zeros(size(I));
for i = 1 : numel(River_colors);
    
    target_intensity(:,:,1) = River_colors{i}(1);  % R
    target_intensity(:,:,2) = River_colors{i}(2);  % G
    target_intensity(:,:,3) = River_colors{i}(3);  % B
    
    unary_i = LAB(I) - LAB(target_intensity);
    unary_i = sum(unary_i.^2, 3);
    unary = min(unary, unary_i);
end

% Scale down
unary = unary/1e3;

% Setup the problem instance
C = Curve_extraction(mesh_map, unary);

%% Define settings
% Use all edges with size <= regularization radius
C.regularization_radius = 2.5;
C.VERBOSE = true;

% rho in paper
C.length_penalty = 0;

% sigma in paper
C.curvature_penalty = 0;

% tau in paper
C.torsion_penalty = 0;
C.use_a_star = use_a_star;

%% 
m = [0.0384 0.1112 0.2790;
    0.6650 0.1675 0.1675;
    0.3726 0.9210 0.1350];

% % Trying different lengths
assert(length(curvature_regs) == 3);

C.length_penalty = 0;
C.curvature_penalty = curvature_regs(1);
curvature_1 = C.solve();


C.curvature_penalty = curvature_regs(2);
curvature_2 = C.solve();

C.curvature_penalty = curvature_regs(3);
curvature_3 = C.solve();

% Display
figure(1);
clf;
imagesc(rgb2gray(I))
colormap gray(256); hold on; axis equal;
hold on;

plot(curvature_1(:,2),curvature_1(:,1),'color', m(1,:) , 'linewidth',5)
plot(curvature_2(:,2),curvature_2(:,1),'color', m(2,:) , 'linewidth',5)
plot(curvature_3(:,2),curvature_3(:,1),'color', m(3,:) , 'linewidth',5)
title('Curvature regularization');

%%
assert(length(length_regs) == 3);

C.curvature_penalty = 0;
C.length_penalty = length_regs(1);
length_1 = C.solve();

C.length_penalty = length_regs(2);
length_2 = C.solve();

C.length_penalty = length_regs(3);
length_3 = C.solve();

%% Display
figure(2);
clf;
imagesc(rgb2gray(I))
colormap gray(256); hold on; axis equal; hold on;
plot(length_1(:,2), length_1(:,1),'color', m(1,:) , 'linewidth',5)
plot(length_2(:,2), length_2(:,1),'color', m(2,:) , 'linewidth',5)
plot(length_3(:,2), length_3(:,1),'color', m(3,:) , 'linewidth',5)
title('Length regularization');
