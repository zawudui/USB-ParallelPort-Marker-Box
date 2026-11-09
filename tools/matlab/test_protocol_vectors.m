function test_protocol_vectors()
% Offline protocol tests using captured frames, including bad frames.
assert(isequal(MarkerBoxProtocol.encode(33,[]),uint8([170 2 33 137])));
assert(isequal(MarkerBoxProtocol.encode(32,[]),uint8([170 2 32 136])));
assert(isequal(MarkerBoxProtocol.encode(16,1),uint8([170 3 16 1 184])));
assert(isequal(MarkerBoxProtocol.encode(16,170),uint8([170 3 16 170 19])));
assert(isequal(MarkerBoxProtocol.encode(17,[]),uint8([170 2 17 185])));
assert(isequal(MarkerBoxProtocol.encode(2,[232 3]),uint8([170 4 2 232 3 71])));
version=uint8([85 6 1 33 2 1 0 112]);
r=MarkerBoxProtocol.decode(version,33);
assert(r.Status==1 && isequal(r.Payload,uint8([2 1 0])));
r=MarkerBoxProtocol.decode(uint8([85 8 1 32 5 2 0 10 0 113]),32);
assert(isequal(r.Payload,uint8([5 2 0 10 0])));
r=MarkerBoxProtocol.decode(uint8([85 3 16 16 86]),16);
assert(r.Status==16); % Busy is not a successful SendMarker ACK.
bad=version; bad(end)=bitxor(bad(end),uint8(1));
mustFail(@()MarkerBoxProtocol.decode(bad,33),'MarkerBox:Checksum');
mustFail(@()MarkerBoxProtocol.decode(version,32),'MarkerBox:Command');
mustFail(@()MarkerBoxProtocol.decode(uint8([85 8 1 32 5]),32),'MarkerBox:Length');
mustFail(@()MarkerBoxProtocol.encode(16,zeros(1,7)),'MarkerBox:PayloadLength');
mustFail(@()MarkerBoxProtocol.encode(16,256),'');
fprintf('PASS: request vectors, captured responses, 0xAA, little-endian and invalid frames.\n');
end
function mustFail(f,expected)
try
    f();
catch problem
    if ~isempty(expected), assert(strcmp(problem.identifier,expected)); end
    return;
end
error('MarkerBox:MissingTestError','Expected this invalid input to fail.');
end
