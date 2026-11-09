function finalStatus = example_01_basic(port)
% 基本发码；运行前关闭其他占用该串口的软件。
if nargin<1, port="COM3"; end
box = MarkerBox(port);
cleanup = onCleanup(@()box.close()); %#ok<NASGU>
fprintf('固件版本：%s\n',box.FirmwareVersion);
box.configurePulse(2,10);
codes = [1,42,170,255];
for code = codes
    box.waitIdle();
    r = box.sendMarker(code);
    fprintf('Marker %3d，ACK往返 %.3f ms\n',code,1000*r.AckRoundTrip_s);
    pause(0.05);
end
box.clearMarker();
finalStatus = box.waitIdle();
disp(finalStatus);
% ACK往返时间不是TTL延迟或脉宽；函数结束自动Clear并释放串口。
end
