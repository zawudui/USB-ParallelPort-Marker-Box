function results = verify_matlab_hardware(port)
% Actual R2024b validation: offline vectors + a small bench hardware check.
if nargin<1, port="COM3"; end
test_protocol_vectors();
box=MarkerBox(port);
cleanup=onCleanup(@()box.close()); %#ok<NASGU>
assert(box.FirmwareVersion=="2.1.0");
box.configurePulse(2,10);
before=box.getStatus();
assert(before.Ready && ~before.Busy && before.AutoPulse && before.PulseMs==2 && before.GapMs==10);
codes=[1 42 170 255];
roundTrip=zeros(size(codes));
for k=1:numel(codes)
    r=box.sendMarker(codes(k));
    assert(r.Status==0 && r.Code==codes(k));
    roundTrip(k)=r.AckRoundTrip_s;
    pause(0.05);
end
% Hold/Busy contract, then explicit Clear and default restoration.
box.clearMarker(); box.waitIdle(); box.setMode("hold");
box.sendMarker(1);
held=box.getStatus(); assert(held.Ready && held.Busy && ~held.AutoPulse);
try
    box.sendMarker(2);
    error('MarkerBox:MissingBusy','Expected Busy in hold mode.');
catch problem
    assert(strcmp(problem.identifier,'MarkerBox:Busy'));
end
box.clearMarker(); box.waitIdle();
box.configurePulse(2,10);
final=box.getStatus();
assert(final.Ready && ~final.Busy && final.AutoPulse && final.PulseMs==2 && final.GapMs==10);
box.close();
assert(any(serialportlist("available")==string(port)), 'Port not released after close.');
results=struct('Passed',true,'MATLAB',version('-release'),'Port',string(port), ...
    'FirmwareVersion',"2.1.0",'SentCodes',codes,'AckRoundTrip_s',roundTrip, ...
    'HoldBusyClearPassed',true,'PortReleased',true,'FinalStatus',final, ...
    'Scope',"Protocol and cleanup checks; no physical waveform capture.");
disp(results);
end
