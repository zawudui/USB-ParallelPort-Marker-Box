classdef MarkerBoxProtocol
    % Binary protocol helpers; no serial connection or hardware writes here.
    methods (Static)
        function frame = encode(command, payload)
            if nargin < 2, payload = []; end
            validateattributes(command, {'numeric'}, {'scalar','real','finite','integer','>=',0,'<=',255});
            validateattributes(payload, {'numeric'}, {'real','finite','integer','>=',0,'<=',255});
            if numel(payload) > 6
                error('MarkerBox:PayloadLength', 'Request payload must contain at most 6 bytes.');
            end
            frame = uint8([170, numel(payload)+2, double(command), reshape(double(payload),1,[])]);
            frame(end+1) = MarkerBoxProtocol.xorBytes(frame);
        end
        function reply = decode(raw, expectedCommand)
            validateattributes(raw, {'numeric'}, {'vector','real','finite','integer','>=',0,'<=',255});
            raw = reshape(uint8(raw),1,[]);
            if numel(raw) < 5 || raw(1) ~= 85
                error('MarkerBox:Header', 'Invalid response header.');
            end
            n = double(raw(2));
            if n < 3 || n > 8 || numel(raw) ~= n+2
                error('MarkerBox:Length', 'Invalid response length.');
            end
            if MarkerBoxProtocol.xorBytes(raw) ~= 0
                error('MarkerBox:Checksum', 'Response XOR checksum failed.');
            end
            if raw(4) ~= expectedCommand
                error('MarkerBox:Command', 'Response belongs to another command.');
            end
            reply = struct('Status',double(raw(3)), 'Command',double(raw(4)), ...
                'Payload',raw(5:end-1), 'Frame',raw);
        end
        function value = xorBytes(bytes)
            value = uint8(0);
            for k = 1:numel(bytes), value = bitxor(value,uint8(bytes(k))); end
        end
    end
end
