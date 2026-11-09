classdef MarkerBox < handle
    % USB-to-DB25 client for MATLAB R2024b. Keep one connection open.
    % All markers use framed binary requests, including code 170 (0xAA).
    % No automatic retry: a missing ACK does not prove no pulse was emitted.
    properties (SetAccess=private)
        Port = ""
        FirmwareVersion = ""
    end
    properties (Access=private)
        Serial = []
        HostClock
    end
    methods
        function obj = MarkerBox(port)
            if nargin < 1, port = "COM3"; end
            port = string(port);
            if ~isscalar(port) || strlength(port)==0
                error('MarkerBox:Port', 'Specify one port, for example COM3.');
            end
            obj.Port = port;
            obj.HostClock = tic;
            try
                obj.Serial = serialport(port,115200, 'DataBits',8, ...
                    'Parity','none', 'StopBits',1, 'FlowControl','none', 'Timeout',2);
                setDTR(obj.Serial,true);
                flush(obj.Serial,"input");
                obj.FirmwareVersion = obj.getVersion();
                obj.waitReady(3);
            catch problem
                obj.Serial = [];
                rethrow(problem);
            end
        end
        function version = getVersion(obj)
            r = obj.transact(33,[],1);
            if numel(r.Payload) ~= 3
                error('MarkerBox:VersionLength', 'Invalid GetVersion payload.');
            end
            version = string(sprintf('%d.%d.%d',r.Payload));
        end
        function state = getStatus(obj)
            r = obj.transact(32,[],1);
            if numel(r.Payload) ~= 5
                error('MarkerBox:StatusLength', 'Invalid GetStatus payload.');
            end
            p = r.Payload;
            pulse = bitand(p(1),uint8(1))~=0;
            if pulse, mode="pulse"; else, mode="hold"; end
            state = struct('Mode',mode, 'AutoPulse',pulse, ...
                'Busy',bitand(p(1),uint8(2))~=0, ...
                'Ready',bitand(p(1),uint8(4))~=0, ...
                'PulseMs',double(p(2))+256*double(p(3)), ...
                'GapMs',double(p(4))+256*double(p(5)));
        end
        function receipt = sendMarker(obj,code)
            validateattributes(code, {'numeric'}, {'scalar','real','finite','integer','>=',0,'<=',255});
            expected = 0;
            if code==0, expected=1; end % Firmware treats zero as Clear.
            receipt = obj.transact(16,code,expected);
            receipt.Code = double(code);
        end
        function receipt = clearMarker(obj)
            receipt = obj.transact(17,[],1);
        end
        function setMode(obj,mode)
            mode = lower(string(mode));
            if ~isscalar(mode) || ~any(mode==["pulse","hold"])
                error('MarkerBox:Mode', 'Mode must be pulse or hold.');
            end
            obj.transact(1,double(mode=="pulse"),1);
        end
        function setPulse(obj,milliseconds)
            validateattributes(milliseconds, {'numeric'}, {'scalar','real','finite','integer','>=',1,'<=',1000});
            obj.transact(2,MarkerBox.u16(milliseconds),1);
        end
        function setGap(obj,milliseconds)
            validateattributes(milliseconds, {'numeric'}, {'scalar','real','finite','integer','>=',0,'<=',1000});
            obj.transact(3,MarkerBox.u16(milliseconds),1);
        end
        function configurePulse(obj,pulseMs,gapMs)
            % This explicit configuration operation clears any held code.
            if nargin<2, pulseMs=2; end
            if nargin<3, gapMs=10; end
            validateattributes(pulseMs, {'numeric'}, {'scalar','real','finite','integer','>=',1,'<=',1000});
            validateattributes(gapMs, {'numeric'}, {'scalar','real','finite','integer','>=',0,'<=',1000});
            obj.clearMarker();
            obj.waitIdle(3);
            obj.setMode("pulse");
            obj.setPulse(pulseMs);
            obj.setGap(gapMs);
        end
        function state = waitIdle(obj,timeoutSeconds)
            if nargin<2, timeoutSeconds=3; end
            validateattributes(timeoutSeconds, {'numeric'}, {'scalar','real','finite','positive'});
            clock = tic;
            while true
                state = obj.getStatus();
                if ~state.Ready
                    error('MarkerBox:NotReady', 'Device is not ready.');
                end
                if ~state.Busy, return; end
                if toc(clock)>=timeoutSeconds
                    error('MarkerBox:IdleTimeout', 'Still busy; a held code requires clearMarker first.');
                end
                pause(0.005);
            end
        end
        function close(obj)
            if isempty(obj.Serial), return; end
            try
                obj.clearMarker();
            catch problem
                warning('MarkerBox:CloseClear', 'Clear was not confirmed before closing: %s',problem.message);
            end
            % Serial is private and has no callback references. Releasing its
            % only reference closes the serialport connection.
            obj.Serial = [];
        end
        function delete(obj)
            obj.close();
        end
    end
    methods (Access=private)
        function state = waitReady(obj,timeoutSeconds)
            clock=tic;
            while true
                state=obj.getStatus();
                if state.Ready, return; end
                if toc(clock)>=timeoutSeconds
                    error('MarkerBox:ReadyTimeout', 'Device did not become ready.');
                end
                pause(0.02);
            end
        end
        function receipt = transact(obj,command,payload,expectedStatus)
            if isempty(obj.Serial)
                error('MarkerBox:Closed', 'Serial connection is closed.');
            end
            request = MarkerBoxProtocol.encode(command,payload);
            submit = toc(obj.HostClock);
            try
                write(obj.Serial,request,"uint8");
                submitted = toc(obj.HostClock);
                header = obj.readExact(2);
                if header(1)~=85 || header(2)<3 || header(2)>8
                    error('MarkerBox:Header', 'Invalid response header or length.');
                end
                raw = [header, obj.readExact(double(header(2)))];
                receipt = MarkerBoxProtocol.decode(raw,command);
            catch problem
                try, flush(obj.Serial,"input"); catch, end
                rethrow(problem);
            end
            acknowledged = toc(obj.HostClock);
            if receipt.Status ~= expectedStatus
                switch receipt.Status
                    case 16, id='MarkerBox:Busy'; message='Device busy. In hold mode, Clear and waitIdle before another code.';
                    case 17, id='MarkerBox:BadParameter'; message='Device rejected a parameter.';
                    case 18, id='MarkerBox:RequestChecksum'; message='Device rejected the request checksum.';
                    case 19, id='MarkerBox:FrameTimeout'; message='Device timed out waiting for a complete request.';
                    case 20, id='MarkerBox:NotReady'; message='Device is not ready.';
                    otherwise, id='MarkerBox:Ack'; message='Unexpected acknowledgement status.';
                end
                error(id,'%s Status=0x%02X. No automatic resend.',message,receipt.Status);
            end
            receipt.RequestFrame = request;
            receipt.HostSubmitTime_s = submit;
            receipt.HostWriteReturnTime_s = submitted;
            receipt.HostAckTime_s = acknowledged;
            receipt.AckRoundTrip_s = acknowledged-submit;
            % These are host timestamps, not a physical TTL edge measurement.
        end
        function bytes = readExact(obj,count)
            bytes = reshape(uint8(read(obj.Serial,count,"uint8")),1,[]);
            if numel(bytes)~=count
                error('MarkerBox:ShortRead', 'Incomplete response. Output may already have occurred; do not blindly resend.');
            end
        end
    end
    methods (Static,Access=private)
        function bytes = u16(value)
            bytes = uint8([mod(double(value),256),floor(double(value)/256)]);
        end
    end
end
