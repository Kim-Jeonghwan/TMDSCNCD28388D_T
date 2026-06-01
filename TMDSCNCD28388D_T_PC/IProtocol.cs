using System;

namespace TMDSCNCD28388D_T_PC
{
    public interface IProtocol
    {
        bool IsConnected { get; }
        event Action<StatusMessageData> OnStatusReceived;
        event Action<string> OnCommError;
        event Action OnPortClosed;
        event Action<byte[]> OnRawTx;
        event Action<byte[]> OnRawRx;

        void Connect(string portName, int baudRate);
        void Disconnect();
        void ReInit();
        void SendControlMessage(ControlMessageData ctrlDto);
    }
}
