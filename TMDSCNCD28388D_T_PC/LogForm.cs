using System;
using System.Drawing;
using System.Windows.Forms;
using System.Collections.Concurrent;

namespace TMDSCNCD28388D_T_PC
{
    public class LogForm : Form
    {
        private ListBox lstLog;
        private ConcurrentQueue<string> _logQueue = new ConcurrentQueue<string>();
        private System.Windows.Forms.Timer _timer;
        private const int MAX_LOGS = 6000;

        public LogForm()
        {
            this.Text = "Real-Time Communication Log History (6000 items)";
            this.Size = new Size(900, 600);
            this.StartPosition = FormStartPosition.CenterScreen;
            this.BackColor = Color.FromArgb(30, 30, 30);
            this.ForeColor = Color.White;

            lstLog = new ListBox
            {
                Dock = DockStyle.Fill,
                BackColor = Color.FromArgb(45, 45, 48),
                ForeColor = Color.LightGreen,
                Font = new Font("Consolas", 11),
                ItemHeight = 18,
                BorderStyle = BorderStyle.None
            };
            this.Controls.Add(lstLog);

            _timer = new System.Windows.Forms.Timer { Interval = 200 };
            _timer.Tick += (s, e) => FlushLogs();
            _timer.Start();
        }

        public void AddLog(string msg)
        {
            _logQueue.Enqueue(msg);
        }

        private void FlushLogs()
        {
            if (_logQueue.IsEmpty) return;

            lstLog.BeginUpdate();
            while (_logQueue.TryDequeue(out string logLine))
            {
                lstLog.Items.Add(logLine);
                if (lstLog.Items.Count > MAX_LOGS)
                {
                    lstLog.Items.RemoveAt(0);
                }
            }
            // Auto scroll
            lstLog.TopIndex = lstLog.Items.Count - 1;
            lstLog.EndUpdate();
        }

        protected override void OnFormClosing(FormClosingEventArgs e)
        {
            e.Cancel = true; // just hide it
            this.Hide();
        }
    }
}
