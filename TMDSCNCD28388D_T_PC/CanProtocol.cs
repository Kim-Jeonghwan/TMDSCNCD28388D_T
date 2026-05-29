using System;
using System.IO.Ports;
using System.Text;
using System.Threading;
using System.Collections.Generic;

namespace TR28386_T_PC
{
    public class CanProtocol : IProtocol
    {
        private SerialPort _serialPort;
        private Thread _readThread;
        private bool _keepReading;

        public event Action<StatusMessageData> OnStatusReceived;
        public event Action<string> OnCommError;
        public event Action OnPortClosed;
        public event Action<byte[]> OnRawTx;
        public event Action<byte[]> OnRawRx;

        private byte _incNumber = 0;
        private string _rxBuffer = "";

        public bool IsConnected => _serialPort != null && _serialPort.IsOpen;

        public void Connect(string portName, int baudRate)
        {
            if (IsConnected) Disconnect();

            // ASCII 모드에서는 시리얼 포트를 사용합니다.
            _serialPort = new SerialPort(portName, baudRate, Parity.None, 8, StopBits.One);
            _serialPort.Open();

            // CAN 컨버터 초기화 (가장 단순한 버전으로 원복)
            _serialPort.Write("S8\r"); // 1Mbps 설정
            Thread.Sleep(300);
            
            _serialPort.Write("O\r");  // 채널 열기
            Thread.Sleep(300);

            // 보낸 명령어 로그 출력
            OnRawTx?.Invoke(Encoding.ASCII.GetBytes("S8\r"));
            OnRawTx?.Invoke(Encoding.ASCII.GetBytes("O\r"));

            _keepReading = true;
            _readThread = new Thread(ReadWorker);
            _readThread.IsBackground = true;
            _readThread.Start();
        }

        public void Disconnect()
        {
            _keepReading = false;
            if (_readThread != null && _readThread.IsAlive)
            {
                _readThread.Join(500);
            }

            if (_serialPort != null && _serialPort.IsOpen)
            {
                _serialPort.Close();
                _serialPort.Dispose();
            }
            OnPortClosed?.Invoke();
        }

        public void ReInit()
        {
            _incNumber = 0;
            _rxBuffer = "";
        }

        public void SendControlMessage(ControlMessageData ctrlDto)
        {
            if (!IsConnected) return;

            try
            {
                byte ledBits = 0;
                for (int i = 0; i < 8; i++) if (ctrlDto.LEDs[i]) ledBits |= (byte)(1 << i);

                // ASCII Packet: e[ID:8][DLC:1][DATA:DLC*2]\r
                StringBuilder sb = new StringBuilder();
                sb.Append("e18FF30AD2"); // Extended, ID 18FF30AD, DLC 2
                sb.Append(_incNumber.ToString("X2"));
                sb.Append(ledBits.ToString("X2"));
                sb.Append("\r");

                string packet = sb.ToString();
                byte[] txBytes = Encoding.ASCII.GetBytes(packet);
                _serialPort.Write(txBytes, 0, txBytes.Length);

                OnRawTx?.Invoke(txBytes);
                _incNumber++;
            }
            catch (Exception ex)
            {
                OnCommError?.Invoke(ex.Message);
            }
        }

        private StatusMessageData _lastStatus = new StatusMessageData();

        private void ReadWorker()
        {
            while (_keepReading)
            {
                try
                {
                    if (_serialPort != null && _serialPort.IsOpen && _serialPort.BytesToRead > 0)
                    {
                        byte[] readBuf = new byte[_serialPort.BytesToRead];
                        int bytesRead = _serialPort.Read(readBuf, 0, readBuf.Length);
                        _rxBuffer += Encoding.ASCII.GetString(readBuf);

                        int crIdx;
                        while ((crIdx = _rxBuffer.IndexOf('\r')) >= 0)
                        {
                            string packet = _rxBuffer.Substring(0, crIdx).Trim();
                            _rxBuffer = _rxBuffer.Substring(crIdx + 1);

                            if (string.IsNullOrEmpty(packet)) continue;

                            OnRawRx?.Invoke(Encoding.ASCII.GetBytes("RX_PKT: " + packet + "\r"));

                            string cmd = packet.Substring(0, 1).ToLower();
                            if ((cmd == "e" || cmd == "t") && packet.Length >= 10)
                            {
                                string idStr = packet.Substring(1, 8).ToUpper();
                                if (idStr == "15555555") 
                                {
                                    ProcessCanPacket(idStr, packet.Substring(10)); 
                                }
                            }
                        }
                    }
                    else
                    {
                        Thread.Sleep(5);
                    }
                }
                catch { Thread.Sleep(10); }
            }
        }

        private void ProcessCanPacket(string idStr, string hexData)
        {
            if (hexData.Length < 4) return;

            try
            {
                int byteLen = hexData.Length / 2;
                byte[] data = new byte[byteLen];
                for (int i = 0; i < byteLen; i++)
                {
                    data[i] = Convert.ToByte(hexData.Substring(i * 2, 2), 16);
                }

                if (idStr == "15555555")
                {
                    _lastStatus.IncNumber = data[0];
                    byte statusBits = data[1];
                    _lastStatus.Tact01 = (statusBits & 0x01) != 0;
                    _lastStatus.Tact02 = (statusBits & 0x02) != 0;
                }

                OnStatusReceived?.Invoke(_lastStatus);
            }
            catch { }
        }
    }
}
