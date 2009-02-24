function water(names, varargin)
% water(names, time, val, time, val, time, val, ...)
%
% Plot given time/value channels in a "Waterfall" plot.
%
% Time needs to be Matlab datevals,
% and the time axis must steadily grow: time(i) < time(i+1).
%
% kasemir@lanl.gov

% disable OpenGL because it doesn't work
opengl neverselect;

channels=(nargin-1)/2;

% Find common time range d0..d1
% This looks more complicated than it is
% because of the variable argument list...
t0=min(varargin{1});
t1=max(varargin{1});
l=length(varargin{1});
minl=l;
for i=2:channels,
    c=2*i-1;
    t0=max(t0, min(varargin{c}));
    l=length(varargin{c});
    minl=min(minl, l);
    t1=min(t1, max(varargin{c}));
end

% Interpolate onto matching dates in d0..d1, minl steps:
T=linspace(t0, t1, minl);
for i=1:channels,
    c=2*(i-1)+1;
    Z(i,:)=interp1(varargin{c}, varargin{c+1}, T);
end

% Plot as colored 3D surface, view from above
surf(T, 1:channels, Z);
colormap(jet);
view(-90, 90);
axis tight;
colorbar;
shading interp;
%shading flat;

title('{\bfArchive Data Display: }{\it"Waterfall"}')
% xlabel([datestr(d0) ' - ' datestr(d1)]);
datetick('x', 2);
set(gca,'YTick', 1:nargin);
set(gca,'YTickLabel', names);
zlabel('value');
rotate3d on;
