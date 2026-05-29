using System;
using System.IO.Ports;
using System.Collections.Concurrent;
using System.Threading;
using System.Threading.Tasks;

namespace TR28386_T_PC
{
    public class StatusMessageData
    {
        public bool Tact01 { get; set; }
        public bool Tact02 { get; set; }
        public bool EthLoop1 { get; set; }
        public bool EthLoop2 { get; set; }
        public double EncoderAngle { get; set; }
        public ushort EncoderRawPD { get; set; }
        public byte EepromReadVal { get; set; }
        public double PWMRaw { get; set; }
        public double PWMRCLPF { get; set; }
        public double PWMBWLPF { get; set; }
        public double PotenRAW { get; set; }
        public double PotenMAVE { get; set; }
        public byte IncNumber { get; set; }
        public bool IsCommError { get; set; }
    }

    public class ControlMessageData
    {
        public bool[] LEDs { get; set; } = new bool[8];
        public bool EepWrite { get; set; }
        public bool EepRead { get; set; }
        public bool Epwm7aEn { get; set; }
        public bool LoopbackTest { get; set; }

        public ushort EepAddr { get; set; }
        public byte EepromWriteVal { get; set; }
        public byte Epwm7aDuty { get; set; }
        public byte Epwm7aFreq { get; set; }
    }

    public class SciPcProtocol : IProtocol
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

        public bool IsConnected => _serialPort != null && _serialPort.IsOpen;

        public void Connect(string portName, int baudRate)
        {
            if (IsConnected) Disconnect();

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
            // 통신 초기화 버튼 동작
            _incNumber = 0;
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
                // Command bits (16-bit)
                ushort commandVal = 0;
                if (ctrlDto.LEDs[0]) commandVal |= (1 << 0);
                if (ctrlDto.LEDs[1]) commandVal |= (1 << 1);
                if (ctrlDto.LEDs[2]) commandVal |= (1 << 2);
                if (ctrlDto.LEDs[3]) commandVal |= (1 << 3);
                if (ctrlDto.LEDs[4]) commandVal |= (1 << 4);
                if (ctrlDto.LEDs[5]) commandVal |= (1 << 5);
                if (ctrlDto.LEDs[6]) commandVal |= (1 << 6);
                if (ctrlDto.LEDs[7]) commandVal |= (1 << 7);
                if (ctrlDto.EepWrite) commandVal |= (1 << 8);
                if (ctrlDto.EepRead) commandVal |= (1 << 9);
                if (ctrlDto.Epwm7aEn) commandVal |= (1 << 10);
                if (ctrlDto.LoopbackTest) commandVal |= (1 << 11);

                byte[] packet = new byte[13];
                packet[0] = 0x7E;
                packet[1] = 0x10;
                packet[2] = 0x09; // Length = Payload(8) + 1 = 9

                // Payload (8 bytes)
                packet[3] = _incNumber;
                packet[4] = (byte)(commandVal & 0xFF);
                packet[5] = (byte)((commandVal >> 8) & 0xFF);
                packet[6] = (byte)(ctrlDto.EepAddr & 0xFF);
                packet[7] = (byte)((ctrlDto.EepAddr >> 8) & 0xFF);
                packet[8] = ctrlDto.EepromWriteVal;
                packet[9] = ctrlDto.Epwm7aDuty;
                packet[10] = ctrlDto.Epwm7aFreq;

                // CRC = Length + Payload Sum
                int crcSum = packet[2]; // Length
                for (int i = 3; i < 11; i++)
                {
                    crcSum += packet[i];
                }
                
                packet[11] = (byte)(crcSum & 0xFF);
                packet[12] = 0x0D;

                _serialPort.Write(packet, 0, packet.Length);
                OnRawTx?.Invoke(packet);
                
                // 순환 증가
                _incNumber++;
                if (_incNumber > 255) _incNumber = 0; 
            }
            catch (Exception ex)
            {
                OnCommError?.Invoke(ex.Message);
            }
        }

        private void ReadWorker()
        {
            var buffer = new System.Collections.Generic.List<byte>();
            
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
                            int totalPacketLen = len + + 4; // SOF, ID, LEN, [Payload+CheckSum...], EOT -> 1(SOF)+1(ID)+1(LEN)+ (LEN bytes of payload+CRC?) Let's check firmware:
                            // MCU sends Length value = 17.
                            // SOF(1) + ID(1) + LEN(1) + PAYLOAD(16) + CRC(1) + EOT(1) = Total 21 bytes.
                            // If LEN=17, total = 17(this encompasses 16 payload + length byte itself... wait)
                            // Firmware calc: Buf[2] = pos - 2 = 19 - 2 = 17. CheckSum is Buf[19].
                            // Payload bytes = 16. Total bytes = 1(SOF) + 1(ID) + 1(LEN) + 16(Payload) + 1(CRC) + 1(EOT) = 21. 
                            // So 1 + 1 + 1 + (len-1) + 1 + 1 = 4 + len. Wait. If len = 17, 4 + 17 = 21. Correct. 
                            totalPacketLen = len + 4;

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
                                buffer.RemoveAt(0);
                                OnCommError?.Invoke("통신 오류 (CRC Error)");
                                continue;
                            }

                            // Parse Payload (from index 3)
                            var status = new StatusMessageData();
                            int pos = 3; // incNumber is buffer[3]
                            status.IncNumber = buffer[pos++];

                            ushort statusBits = buffer[pos++];
                            status.Tact01 = (statusBits & 0x01) != 0;
                            status.Tact02 = (statusBits & 0x02) != 0;
                            status.EthLoop1 = (statusBits & 0x04) != 0;
                            status.EthLoop2 = (statusBits & 0x08) != 0;

                            ushort encAngleRaw = (ushort)(buffer[pos++] | (buffer[pos++] << 8));
                            status.EncoderAngle = encAngleRaw / 100.0;

                            status.EncoderRawPD = buffer[pos++];
                            status.EepromReadVal = buffer[pos++];

                            ushort pwmRaw = (ushort)(buffer[pos++] | (buffer[pos++] << 8));
                            status.PWMRaw = pwmRaw / 1000.0;

                            ushort pwmclpf = (ushort)(buffer[pos++] | (buffer[pos++] << 8));
                            status.PWMRCLPF = pwmclpf / 1000.0;

                            ushort pwmbwlpf = (ushort)(buffer[pos++] | (buffer[pos++] << 8));
                            status.PWMBWLPF = pwmbwlpf / 1000.0;

                            ushort potenRaw = (ushort)(buffer[pos++] | (buffer[pos++] << 8));
                            status.PotenRAW = potenRaw / 1000.0;

                            ushort potenMave = (ushort)(buffer[pos++] | (buffer[pos++] << 8));
                            status.PotenMAVE = potenMave / 1000.0;
                            
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
