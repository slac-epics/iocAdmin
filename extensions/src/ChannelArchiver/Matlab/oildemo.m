% "Oil Tank" plot demo
%
% Plots the temperatures of the oil tanks
% for the RFQ, DTL1, ... DTL6 SNS Transmitters
% for March 2004 every which way from Sunday.
%
% Shows some of the things you can do in Matlab
% with the data, also shows that there's nothing
% automagic: You get the archive data and need
% to deal with it, sample by sample and channel
% by channel.

% Setup url/key
% You need to adjust this to point to a network
% data server that's serving ChannelArchiver/DemoData/index
url='http://localhost/cgi-bin/xmlrpc/ArchiveDataServer.cgi';
key=4;

% Can we connect?
ml_arch_info(url);
% Request methods must match
% what ml_arch_info showed:
h_raw=0;
h_average=2;
h_plotbin=3;

% Check if the key works by comparing output
% of this:
ml_arch_names(url, key, 'Tnk.:T$');
% ... with the names & times that we use:
names={ 'RFQ_HPRF:Tnk1:T', ...
        'DTL_HPRF:Tnk1:T', ...
        'DTL_HPRF:Tnk2:T', ...
        'DTL_HPRF:Tnk3:T', ...
        'DTL_HPRF:Tnk4:T', ...
        'DTL_HPRF:Tnk5:T', ...
        'DTL_HPRF:Tnk6:T' };
t0=datenum(2004,3,1);
t1=datenum(2004,3,30);

% Get plot-binned for preview
figure(1);
tic;
[rfq,dtl1,dtl2,dtl3,dtl4,dtl5,dtl6]=ArchiveData(url, 'values', key, names,...
                                           t0, t1, 500, h_plotbin);
secs=toc
figure(1);
plot(rfq(1,:),rfq(3,:),...
     dtl1(1,:),dtl1(3,:),...
     dtl2(1,:),dtl2(3,:),...
     dtl3(1,:),dtl3(3,:),...
     dtl4(1,:),dtl4(3,:),...
     dtl5(1,:),dtl5(3,:),...
     dtl6(1,:),dtl6(3,:)...
     );
datetick('x',7);grid on;
ylim([60 120]);
legend('RFQ', 'DTL1','DTL2', 'DTL3', 'DTL4', 'DTL5', 'DTL6');
xlabel('Date in March 2004');
ylabel('T [Deg. F]');
title({'{\bfSNS Transmitter Oil Tank Temperatures}', '{\it(Data retrieved in 500 "Plot-Bins")}'});


% Get averages:
% Hides detail but might be more convenient for post-processing
tic;
[rfq,dtl1,dtl2,dtl3,dtl4,dtl5,dtl6]=ArchiveData(url, 'values', key, names,...
                                           t0, t1, 1000, h_average);
secs=toc
figure(2);
plot(rfq(1,:),(rfq(3,:)-32)*5/9,...
     dtl1(1,:),(dtl1(3,:)-32)*5/9,...
     dtl2(1,:),(dtl2(3,:)-32)*5/9,...
     dtl3(1,:),(dtl3(3,:)-32)*5/9,...
     dtl4(1,:),(dtl4(3,:)-32)*5/9,...
     dtl5(1,:),(dtl5(3,:)-32)*5/9,...
     dtl6(1,:),(dtl6(3,:)-32)*5/9)
datetick('x',7);grid on;
ylim([15 50]);
legend('RFQ', 'DTL1','DTL2', 'DTL3', 'DTL4', 'DTL5', 'DTL6');
xlabel('Date in March 2004');
ylabel('T [^o C]');
title({'{\bfSNS Transmitter Oil Tank Temperatures}', '{\it(Data averaged into 1000 bins)}'});

figure(3);
N=1500;
% We could request the data in one call like this:
%[rfq,dtl1,dtl2,dtl3,dtl4,dtl5,dtl6]=ArchiveData(url, 'values', key, names,...
%                                           t0, t1, N, h_average);
% Problem: That will result in a spreadsheet-filled
%          data set where all time stamps match acros channels.
%          The filling around 'Archive Off' etc. will result
%          in points on the time axis where the time stamps are repeated,
%          and interp1 as used in 'water' cannot handle that.
% So we request the data channel by channel:
rfq=ArchiveData(url, 'values', key, names{1}, t0, t1, N, h_average);
dtl1=ArchiveData(url, 'values', key, names{2}, t0, t1, N, h_average);
dtl2=ArchiveData(url, 'values', key, names{3}, t0, t1, N, h_average);
dtl3=ArchiveData(url, 'values', key, names{4}, t0, t1, N, h_average);
dtl4=ArchiveData(url, 'values', key, names{5}, t0, t1, N, h_average);
dtl5=ArchiveData(url, 'values', key, names{6}, t0, t1, N, h_average);
dtl6=ArchiveData(url, 'values', key, names{7}, t0, t1, N, h_average);
labels={ 'DTL6', 'DTL5', 'DTL4', 'DTL3', 'DTL2', 'DTL1', 'RFQ' };
water(labels, ...
      dtl6(1,:),dtl6(3,:), ...
      dtl5(1,:),dtl5(3,:), ...
      dtl4(1,:),dtl4(3,:), ...
      dtl3(1,:),dtl3(3,:), ...
      dtl2(1,:),dtl2(3,:), ...
      dtl1(1,:),dtl1(3,:),...
      rfq(1,:),rfq(3,:));
title({'{\bf"Waterfall" Plot of SNS Transmitter Oil Tank Temperatures}', '{\it(Data averaged into 1000 bins)}'});


figure(4);
water(labels, ...
      dtl6(1,:),dtl6(3,:), ...
      dtl5(1,:),dtl5(3,:), ...
      dtl4(1,:),dtl4(3,:), ...
      dtl3(1,:),dtl3(3,:), ...
      dtl2(1,:),dtl2(3,:), ...
      dtl1(1,:),dtl1(3,:),...
      rfq(1,:),rfq(3,:));
set(colorbar, 'Visible', 'off');
title({'{\bfSurface Plot of SNS Transmitter Oil Tank Temperatures}', '{\it(Data averaged into 1000 bins)}'});
zlabel('Temperature [Deg. F]');
zlim([75 120]);
view([-45, 30]);
