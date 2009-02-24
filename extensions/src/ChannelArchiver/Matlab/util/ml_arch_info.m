function [ver, desc, hows]=ml_arch_info(url)
% [ver, desc, hows]=ml_arch_info(url)
%
% Get archive server info,
% printing it unless return values are used.

global is_matlab
[ver, desc, hows]=ArchiveData(url, 'info');
if nargout < 1
    disp(desc)
    for i=1:size(hows,1)
        if is_matlab==1
            disp(sprintf('How=%d: %s', i-1, hows{i}));
        else
            disp(sprintf('How=%d: %s', i-1, hows(i,:)));
        end
    end
end
