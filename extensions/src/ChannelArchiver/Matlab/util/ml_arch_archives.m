function [keys,names,paths]=ml_arch_archives(url)
% [keys,names,paths]=ml_arch_archives(url)
%
% Get list of available archives from server,
% printing it unless return values are used.

global is_matlab
[keys,names,paths]=ArchiveData(url, 'archives');
if nargout < 1
    for i=1:size(keys,1)
        if is_matlab==1
            disp(sprintf('Key %d: %s (%s)', keys(i), names{i}, paths{i}));
        else
            disp(sprintf('Key %d: %s (%s)', keys(i), names(i,:), paths(i,:)));
        end
    end
end;
