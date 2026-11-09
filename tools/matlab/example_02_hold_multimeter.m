function example_02_hold_multimeter(port)
% 交互式万用表测试。仅台架使用，DB25 不接实验设备。
% 黑笔接隔离地Pin25；数据Pin2..9为D0..D7。方向见使用说明。
if nargin<1, port="COM3"; end
box = MarkerBox(port);
cleanup = onCleanup(@()restoreAndClose(box)); %#ok<NASGU>
box.clearMarker();
box.waitIdle();
box.setMode("hold");
codes = [0,1,2,4,8,16,32,64,128,255];
for code = codes
    box.clearMarker();
    box.waitIdle();
    if code~=0, box.sendMarker(code); end
    fprintf('\n保持 0x%02X；D7..D0 = %s\n',code,dec2bin(code,8));
    if code>0 && code<255
        bitNumber=log2(code);
        fprintf('预期D%d / DB25 Pin%d / 正面上排从右第%d孔为高。\n',bitNumber,bitNumber+2,bitNumber+2);
    end
    input('测完后按回车继续（Ctrl+C结束并执行清理）：','s');
end
box.clearMarker();
fprintf('已清零；结束时恢复自动脉冲2ms/间隙10ms并关闭连接。\n');
end
function restoreAndClose(box)
try
    box.configurePulse(2,10);
catch problem
    warning('MarkerBox:Restore','默认参数恢复未确认：%s',problem.message);
end
box.close();
end
