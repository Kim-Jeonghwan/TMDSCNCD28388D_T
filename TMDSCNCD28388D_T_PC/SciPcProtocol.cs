using System;
using System.IO.Ports;
using System.Collections.Generic;
using System.Threading;

namespace TMDSCNCD28388D_T_PC
{
    public class StatusMessageData
    {
        public byte IncNumber { get; set; }
        public byte Status { get; set; }
        public double DspTemp { get; set; }
        public bool IsCommError { get; set; }
    }

    public class ControlMessageData
    {
        public byte ManualSeqNum { get; set; }
    }

    public class SciPcProtocol : IProtocol
    {
        private SerialPort _serialPort;
        private Thread _readThread;
        private bool _keepReading;
        
        public event Action<StatusMessageData> OnStatusReceived;
        public event Action<string> OnCommError;
        public event Action OnPortClosed;
        public event Action<int> OnCrcErrorCountUpdated; // CRC 에러 발생 횟수 알림
        
        public event Action<byte[]> OnRawTx;
        public event Action<byte[]> OnRawRx;

        public bool IsConnected => _serialPort != null && _serialPort.IsOpen;
        
        private int _crcErrorCount = 0;

        public void Connect(string portName, int baudRate)
        {
            if (IsConnected) Disconnect();

            _crcErrorCount = 0;
            OnCrcErrorCountUpdated?.Invoke(_crcErrorCount);

            _serialPort = new SerialPort(portName, baudRate, Parity.None, 8, StopBits.One);
            _serialPort.Open();

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
            _crcErrorCount = 0;
            OnCrcErrorCountUpdated?.Invoke(_crcErrorCount);
            
            if (_serialPort != null && _serialPort.IsOpen)
            {
                _serialPort.DiscardInBuffer();
                _serialPort.DiscardOutBuffer();
            }
        }

        public void SendControlMessage(ControlMessageData ctrlDto)
        {
            if (!IsConnected) return;

            try
            {
                // Packet Size: SOF(1) + ID(1) + LEN(1) + Payload(2) + CRC(1) + EOT(1) = 7 bytes
                byte[] packet = new byte[7];
                packet[0] = 0x7E; // SOF
                packet[1] = 0x10; // ID
                packet[2] = 0x03; // LEN = Payload(2) + 1

                // Payload (2 bytes)
                packet[3] = ctrlDto.ManualSeqNum;
                packet[4] = 0x00; // Command (Reserved 1~8) - unused, set to 0

                // CheckSum
                int crcSum = packet[2]; // start with LEN
                for (int i = 3; i < 5; i++)
                {
                    crcSum += packet[i];
                }
                
                packet[5] = (byte)(crcSum & 0xFF);
                packet[6] = 0x0D; // EOT

                _serialPort.Write(packet, 0, packet.Length);
                OnRawTx?.Invoke(packet);
            }
            catch (Exception ex)
            {
                OnCommError?.Invoke(ex.Message);
            }
        }

        private void ReadWorker()
        {
            var buffer = new List<byte>();
            
            while (_keepReading)
            {
                try
                {
                    if (_serialPort != null && _serialPort.IsOpen && _serialPort.BytesToRead > 0)
                    {
                        byte[] readBuf = new byte[1024];
                        int bytesRead = _serialPort.Read(readBuf, 0, readBuf.Length);
                        for (int i = 0; i < bytesRead; i++)
                        {
                            buffer.Add(readBuf[i]);
                        }

                        // Parse buffer
                        while (buffer.Count > 0)
                        {
                            // Sync up to SOF
                            while (buffer.Count > 0 && buffer[0] != 0x7E)
                            {
                                buffer.RemoveAt(0);
                            }

                            if (buffer.Count < 3) break; // Not enough bytes for Header

                            if (buffer[1] != 0x10)
                            {
                                buffer.RemoveAt(0); // Invalid ID
                                continue;
                            }

                            byte len = buffer[2];
                            // For MCU -> PC Msg1, LEN should be 5.
                            // Total length = 4 (Header/EOT base) + len = 4 + 5 = 9.
                            int totalPacketLen = len + 4; 

                            if (buffer.Count < totalPacketLen) break; // Wait for more

                            // Verify EOT
                            if (buffer[totalPacketLen - 1] != 0x0D)
                            {
                                buffer.RemoveAt(0);
                                OnCommError?.Invoke("통신 오류 (EOT Error)");
                                continue;
                            }

                            // Verify Checksum
                            int calcCrc = buffer[2]; // start with LEN
                            for (int i = 3; i < totalPacketLen - 2; i++)
                            {
                                calcCrc += buffer[i];
                            }
                            byte receivedCrc = buffer[totalPacketLen - 2];

                            if ((calcCrc & 0xFF) != receivedCrc)
                            {
                                buffer.RemoveAt(0); // Drop the packet header and re-sync
                                _crcErrorCount++;
                                OnCrcErrorCountUpdated?.Invoke(_crcErrorCount);
                                OnCommError?.Invoke($"통신 오류 (CRC Error) - 누적: {_crcErrorCount}");
                                continue; // 점검 프로그램 기준, CRC 통과 실패 시 패킷 수신 거부 (Drop)
                            }

                            // Parse Payload (from index 3)
                            var status = new StatusMessageData();
                            int pos = 3; 
                            
                            status.IncNumber = buffer[pos++];
                            status.Status = buffer[pos++];

                            // DspTemp is uint16_t (2 bytes, Little Endian)
                            ushort tempRaw = (ushort)(buffer[pos++] | (buffer[pos++] << 8));
                            // DSP 측에서 소수점 1자리를 위해 x10 해서 보냈으므로 10.0으로 나눔
                            status.DspTemp = tempRaw / 10.0;
                            
                            status.IsCommError = false;

                            // Dispatch
                            OnStatusReceived?.Invoke(status);

                            byte[] rawPacketData = new byte[totalPacketLen];
                            buffer.CopyTo(0, rawPacketData, 0, totalPacketLen);
                            OnRawRx?.Invoke(rawPacketData);

                            // Consume packet
                            buffer.RemoveRange(0, totalPacketLen);
                        }
                    }
                    else
                    {
                        Thread.Sleep(10);
                    }
                }
                catch (TimeoutException) { }
                catch (Exception)
                {
                    // Ignore disconnect IO errors
                    Thread.Sleep(50);
                }
            }
        }
    }
}
