function log = example_03_sequence(port,count,intervalSeconds)
% 少量连续发码并记录ACK；此记录不证明采集端零漏码或2ms物理脉宽。
% pause是每次收到ACK后的等待，不保证精确的Marker起始间隔。
if nargin<1, port="COM3"; end
if nargin<2, count=20; end
if nargin<3, intervalSeconds=0.05; end
validateattributes(count,{'numeric'},{'scalar','integer','>=',1,'<=',10000});
validateattributes(intervalSeconds,{'numeric'},{'scalar','real','finite','>=',0.02});
box = MarkerBox(port);
cleanup = onCleanup(@()box.close()); %#ok<NASGU>
box.configurePulse(2,10);
pattern=[1,2,4,8,16,32,64,128,85,170,255];
Code=zeros(count,1); HostSubmitTime_s=zeros(count,1); AckRoundTrip_ms=zeros(count,1);
for k=1:count
    Code(k)=pattern(mod(k-1,numel(pattern))+1);
    r=box.sendMarker(Code(k));
    HostSubmitTime_s(k)=r.HostSubmitTime_s;
    AckRoundTrip_ms(k)=1000*r.AckRoundTrip_s;
    pause(intervalSeconds);
end
box.clearMarker();
box.waitIdle();
log=table((1:count)',Code,HostSubmitTime_s,AckRoundTrip_ms, ...
    'VariableNames',{'Trial','Code','HostSubmitTime_s','AckRoundTrip_ms'});
disp(log);
% 按需保存：writetable(log,"marker_command_log.csv");
end
