using System;
using System.IO.Ports;
using System.Text;
using System.Threading;
using System.Collections.Generic;

namespace TMDSCNCD28388D_T_PC
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

        private string _rxBuffer = "";

        public bool IsConnected => _serialPort != null && _serialPort.IsOpen;

        public void Connect(string portName, int baudRate)
        {
            if (IsConnected) Disconnect();

            _serialPort = new SerialPort(portName, baudRate, Parity.None, 8, StopBits.One);
            _serialPort.Open();

            _serialPort.Write("S8\r"); 
            Thread.Sleep(300);
            
            _serialPort.Write("O\r");  
            Thread.Sleep(300);

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
            _rxBuffer = "";
        }

        public void SendControlMessage(ControlMessageData ctrlDto)
        {
            if (!IsConnected) return;

            try
            {
                // ASCII Packet: e[ID:8][DLC:1][DATA:DLC*2]\r
                StringBuilder sb = new StringBuilder();
                sb.Append("e18FF30AD1"); // Extended, ID 18FF30AD, DLC 1
                sb.Append(ctrlDto.ManualSeqNum.ToString("X2"));
                sb.Append("\r");

                string packet = sb.ToString();
                byte[] txBytes = Encoding.ASCII.GetBytes(packet);
                _serialPort.Write(txBytes, 0, txBytes.Length);

                OnRawTx?.Invoke(txBytes);
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
            if (hexData.Length < 2) return;

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
                    if (data.Length >= 2)
                    {
                        _lastStatus.Status = data[1];
                    }
                }

                OnStatusReceived?.Invoke(_lastStatus);
            }
            catch { }
        }
    }
}
