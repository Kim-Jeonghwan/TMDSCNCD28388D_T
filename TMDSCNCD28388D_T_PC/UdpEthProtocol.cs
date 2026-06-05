// UdpEthProtocol.cs
// Nexcom Co., Ltd.
// Description : UDP 이더넷 프로토콜 구현 (규격서 Payload/ACK MSG Format 준수)
// Last Updated : 2026. 06. 02. (PC 수신 포트를 50002로 변경하여 포트 충돌 방지 및 소켓 안정화)
//
// [메시지 종류]
//   PC→DSP: Update 타입 (Request Ack = 0x01, ACK 필요)
//   DSP→PC: Reflect 타입 (Request Ack = 0xFF, ACK 미요청)
//   DSP→PC: ACK 응답 타입 (Code = 0xFF)
//
// [재전송 규칙]
//   - 100ms 내 ACK 없으면 재전송 (Send Count 1→2→3→4)
//   - 4회 모두 실패 시 통신 두절 판단

using System;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Threading.Tasks;

namespace TMDSCNCD28388D_T_PC
{
    /// <summary>UDP 이더넷 프로토콜 구현 클래스 (규격서 Payload/ACK MSG Format 준수)</summary>
    public class UdpEthProtocol : IProtocol
    {
        // ── 네트워크 설정 ─────────────────────────────────────
        private const string DspIpAddress   = "192.168.200.10";
        private const string LocalIpAddress = "192.168.200.100";
        private const int    DspRxPort      = 5001;  // DSP 수신 포트
        private const int    PcRxPort       = 50002; // PC  수신 포트 (5000번 대역 충돌 회피를 위해 변경)

        // ── 규격서 프로토콜 상수 ──────────────────────────────
        private const byte SrcIdPc          = 0x20;
        private const byte DstIdDsp         = 0x10;
        private const byte MsgCodeMonitor   = 0x10;
        private const byte MsgCodeAck       = 0xFF;
        private const byte ReqAckRequest    = 0x01;  // Update 타입
        private const byte ReqAckNone       = 0xFF;  // Reflect 타입
        private const byte PriorityNormal   = 0x02;
        private const byte AckOk            = 0x10;
        private const byte AckNack          = 0x11;

        // ── 재전송 규칙 ───────────────────────────────────────
        private const int   AckTimeoutMs    = 100;   // 100ms ACK 타임아웃
        private const int   MaxSendCount    = 4;     // 최대 전송 횟수 (1회 + 재전송 3회)

        // ── 내부 상태 ─────────────────────────────────────────
        private UdpClient       _udpClient;
        private IPEndPoint      _dspEndPoint;
        private IPEndPoint      _localEndPoint;
        private Thread          _rxThread;
        private volatile bool   _keepReceiving;
        private volatile bool   _ackReceived;
        private volatile bool   _commError;
        private byte            _currentSendCount;
        private readonly object _sendLock = new object();

        // ── IProtocol 이벤트 ──────────────────────────────────
        public event Action<StatusMessageData> OnStatusReceived;
        public event Action<string>            OnCommError;
        public event Action                    OnPortClosed;
        public event Action<byte[]>            OnRawTx;
        public event Action<byte[]>            OnRawRx;
        public bool IsConnected => _udpClient != null;

        /// <summary>
        /// UDP 소켓 연결 (portName 파라미터는 IProtocol 호환용으로 무시, baudRate 무시)
        /// 실제 IP/포트는 클래스 상수 사용
        /// </summary>
        public void Connect(string portName, int baudRate)
        {
            if (IsConnected) Disconnect();

            _commError        = false;
            _ackReceived      = false;
            _currentSendCount = 1;
            /* IPAddress.Any 대신 LocalIpAddress(192.168.100.100)로 강제 바인딩하여 Wi-Fi로 패킷이 새는 것을 방지 */
            _localEndPoint    = new IPEndPoint(IPAddress.Parse(LocalIpAddress), PcRxPort);
            _dspEndPoint      = new IPEndPoint(IPAddress.Parse(DspIpAddress), DspRxPort);

            _udpClient = new UdpClient();
            _udpClient.Client.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReuseAddress, true);
            _udpClient.Client.Bind(_localEndPoint);

            _keepReceiving = true;
            _rxThread = new Thread(ReceiveWorker) { IsBackground = true };
            _rxThread.Start();
        }

        /// <summary>UDP 소켓 해제</summary>
        public void Disconnect()
        {
            _keepReceiving = false;

            try
            {
                _udpClient?.Close();
            }
            catch (Exception) { /* 무시 */ }

            _rxThread?.Join(500);
            _udpClient = null;
            OnPortClosed?.Invoke();
        }

        /// <summary>재초기화 (Disconnect 후 재Connect)</summary>
        public void ReInit()
        {
            string ip   = DspIpAddress;
            int    port = DspRxPort;
            Disconnect();
            Connect(ip, port);
        }

        /// <summary>
        /// PC→DSP Update 메시지 전송 (규격서: ACK 필요, 100ms 내 ACK 없으면 재전송)
        /// Send Count가 MaxSendCount 초과 시 통신 두절 판단
        /// </summary>
        public void SendControlMessage(ControlMessageData ctrlDto)
        {
            if (_udpClient == null) return;

            lock (_sendLock)
            {
                _ackReceived      = false;
                _currentSendCount = 1;

                while (_currentSendCount <= MaxSendCount)
                {
                    byte[] payload = BuildUpdatePayload(ctrlDto.ManualSeqNum,
                                                        cmd: 0x00,
                                                        sendCount: _currentSendCount);
                    SendPayload(payload);

                    // 100ms ACK 대기
                    bool ack = WaitForAck(AckTimeoutMs);
                    if (ack)
                    {
                        _commError = false;
                        break;
                    }

                    _currentSendCount++;
                }

                if (_currentSendCount > MaxSendCount)
                {
                    _commError = true;
                    OnCommError?.Invoke("통신 두절: DSP로부터 ACK 없음 (재전송 3회 초과)");
                }
            }
        }

        // ── 내부: ACK 대기 ────────────────────────────────────
        private bool WaitForAck(int timeoutMs)
        {
            int elapsed = 0;
            const int step = 5;
            while (elapsed < timeoutMs)
            {
                if (_ackReceived) return true;
                Thread.Sleep(step);
                elapsed += step;
            }
            return false;
        }

        // ── 내부: UDP 수신 스레드 ─────────────────────────────
        private void ReceiveWorker()
        {
            IPEndPoint remoteEp = new IPEndPoint(IPAddress.Any, 0);
            while (_keepReceiving)
            {
                try
                {
                    byte[] data = _udpClient?.Receive(ref remoteEp);
                    if (data == null || data.Length == 0) continue;

                    OnRawRx?.Invoke(data);
                    ProcessReceivedUdpPayload(data);
                }
                catch (SocketException)
                {
                    if (_keepReceiving)
                        OnCommError?.Invoke("UDP 수신 오류");
                    break;
                }
                catch (ObjectDisposedException) { break; }
            }
        }

        // ── 내부: 수신 페이로드 파싱 ─────────────────────────
        private void ProcessReceivedUdpPayload(byte[] data)
        {
            // 최소 길이: MSG Header(12B) + 최소 Data(2B) + Checksum(2B) = 16B
            if (data.Length < 16) return;

            byte srcId   = data[4];
            byte code    = data[6];
            byte reqAck  = data[7];

            // ① DSP→PC Reflect 메시지 (온도+시퀀스)
            if (srcId == DstIdDsp && code == MsgCodeMonitor && reqAck == ReqAckNone)
            {
                // Reflect 메시지 구조: 헤더(12) + Data(4: Seq, Status, Temp 2B) + Checksum(2) = 총 18바이트
                if (!VerifyChecksum(data, 18)) return;

                byte   seqNum  = data[12];
                byte   status  = data[13];
                ushort tempRaw = (ushort)(data[14] | (data[15] << 8)); // Little Endian

                var msg = new StatusMessageData
                {
                    IncNumber   = seqNum,
                    Status      = status,
                    DspTemp     = tempRaw / 10.0,
                    IsCommError = _commError
                };
                OnStatusReceived?.Invoke(msg);
            }
            // ② DSP→PC ACK 응답
            else if (srcId == DstIdDsp && code == MsgCodeAck)
            {
                byte ackResult = reqAck;  // 0x10=ACK, 0x11=NACK
                if (ackResult == AckOk)
                {
                    _ackReceived = true;
                    _commError   = false;
                }
                else if (ackResult == AckNack)
                {
                    // NACK 수신 시 통신 오류 보고
                    OnCommError?.Invoke("NACK 수신: DSP Checksum 오류");
                }
            }
        }

        // ── 내부: PC→DSP Update 페이로드 조립 ───────────────
        private byte[] BuildUpdatePayload(byte seqNum, byte cmd, byte sendCount)
        {
            // Payload = MSG Header(12B) + Data(2B) + Checksum(2B) = 16B
            byte[] payload = new byte[16];
            uint   ts      = (uint)(Environment.TickCount & 0x7FFFFFFF);

            // Timestamp (Little Endian, 4B)
            payload[0] = (byte)(ts & 0xFF);
            payload[1] = (byte)((ts >> 8)  & 0xFF);
            payload[2] = (byte)((ts >> 16) & 0xFF);
            payload[3] = (byte)((ts >> 24) & 0xFF);
            payload[4] = SrcIdPc;
            payload[5] = DstIdDsp;
            payload[6] = MsgCodeMonitor;
            payload[7] = ReqAckRequest;   // Update 타입: 0x01
            payload[8] = PriorityNormal;
            payload[9] = sendCount;
            // Data Length = 2 (Little Endian)
            payload[10] = 0x02;
            payload[11] = 0x00;
            // Data (2B)
            payload[12] = seqNum;
            payload[13] = cmd;
            // Checksum (2B, Little Endian): 앞 14B 합산 최하위 2B
            ushort chk = CalcChecksum(payload, 14);
            payload[14] = (byte)(chk & 0xFF);
            payload[15] = (byte)((chk >> 8) & 0xFF);

            return payload;
        }

        // ── 내부: 규격서 Checksum 계산 ───────────────────────
        private static ushort CalcChecksum(byte[] buf, int length)
        {
            uint sum = 0;
            for (int i = 0; i < length; i++)
                sum += buf[i];
            return (ushort)(sum & 0xFFFF);
        }

        // ── 내부: 수신 Checksum 검증 ─────────────────────────
        private static bool VerifyChecksum(byte[] data, int totalLen)
        {
            if (data.Length < totalLen) return false;
            int   chkOffset = totalLen - 2;
            ushort recvChk  = (ushort)(data[chkOffset] | (data[chkOffset + 1] << 8));
            ushort calcChk  = CalcChecksum(data, chkOffset);
            return recvChk == calcChk;
        }

        // ── 내부: UDP 페이로드 전송 ───────────────────────────
        private void SendPayload(byte[] payload)
        {
            try
            {
                _udpClient?.Send(payload, payload.Length, _dspEndPoint);
                OnRawTx?.Invoke(payload);
            }
            catch (Exception ex)
            {
                OnCommError?.Invoke($"UDP 전송 오류: {ex.Message}");
            }
        }
    }
}
