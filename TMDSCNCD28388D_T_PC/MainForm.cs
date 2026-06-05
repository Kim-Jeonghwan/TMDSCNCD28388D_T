/*
 * File: MainForm.cs
 * Created: 2026-06-01 (Modified by Antigravity)
 * Description: TMDSCNCD28388D_T PC Monitoring Dashboard MainForm
 * Last Updated: 2026. 06. 01. (SCI/UDP 프로토콜 선택 RadioButton 및 통신 두절 UI 추가)
 */

using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.IO.Ports;
using System.Windows.Forms;
using System.Management;
using System.Text.RegularExpressions;
using System.Collections.Generic;
using System.Linq;

namespace TMDSCNCD28388D_T_PC
{
    public class MainForm : Form
    {
        private IProtocol _protocol;
        private System.Windows.Forms.Timer _timer;

        private Color colorBg = Color.FromArgb(30, 30, 30);
        private Color colorPanelBg = Color.FromArgb(45, 45, 48);
        private Color colorText = Color.FromArgb(220, 220, 220);
        private Color colorAccent = Color.FromArgb(0, 122, 204);
        private Color colorError = Color.FromArgb(200, 50, 50);

        private DateTime _lastRxTime = DateTime.MinValue;

        private ControlMessageData _ctrlDto = new ControlMessageData();

        private ComboBox cmbPorts;
        private ComboBox cmbBauds;
        private Button btnConnect;
        private Button btnDisconnect;
        private Button btnInit;
        private Button btnRefresh;
        private Label lblPortConnected;
        private Label lblCommReceiving;

        // Status & Control UI
        private Label lblSeqNumber;
        private Label lblBoardTemp;
        private Label lblCrcErrors;
        
        private TextBox txtInputSeq;
        private Button btnSendSeq;

        // Log
        private LogForm _logForm;
        private Label lblLogRxInfo;
        private Label lblLogTxInfo;

        // 프로토콜 선택 UI
        private RadioButton rdoSci;
        private RadioButton rdoUdp;
        private Label lblCommStatus;  // 통신 두절 표시


        public MainForm()
        {
            this.Text = "TMDSCNCD28388D_T Monitoring & Dashboard";
            this.Size = new Size(1100, 900); // Compact Size
            this.MinimumSize = new Size(800, 600);
            this.StartPosition = FormStartPosition.CenterScreen;
            this.BackColor = colorBg;
            this.ForeColor = colorText;
            this.Font = new Font("맑은 고딕", 9F);

            _logForm = new LogForm();

            _protocol = new SciPcProtocol();
            SetupProtocolEvents();
            SetupMenu();

            _timer = new System.Windows.Forms.Timer { Interval = 100 }; // 100ms UI Update
            _timer.Tick += Timer_Tick;

            BuildUI();
        }

        private void BuildUI()
        {
            TableLayoutPanel mainLayout = new TableLayoutPanel
            {
                Dock = DockStyle.Fill,
                RowCount = 4,
                ColumnCount = 1,
                Padding = new Padding(15, 40, 15, 15)
            };
            mainLayout.RowStyles.Add(new RowStyle(SizeType.Absolute, 220)); // Comm Panel
            mainLayout.RowStyles.Add(new RowStyle(SizeType.Absolute, 200)); // Status Panel
            mainLayout.RowStyles.Add(new RowStyle(SizeType.Absolute, 150)); // Control Panel
            mainLayout.RowStyles.Add(new RowStyle(SizeType.Percent, 100));  // Log Panel

            // 1. Top Bar - Communication Panel
            Panel pnlComm = CreateStyledPanel("COMMUNICATION (PORT / BAUD RATE)");
            pnlComm.Dock = DockStyle.Fill;
            pnlComm.Margin = new Padding(5);

            int commY = 50;
            Label lblPort = new Label { Text = "COM Port", Location = new Point(20, commY), AutoSize = true, Font = new Font("Segoe UI", 11, FontStyle.Bold) };
            cmbPorts = new ComboBox
            {
                Location = new Point(160, commY - 2),
                Size = new Size(350, 35),
                BackColor = Color.FromArgb(45, 45, 48),
                ForeColor = Color.FromArgb(0, 255, 200),
                DropDownStyle = ComboBoxStyle.DropDownList,
                Font = new Font("맑은 고딕", 11)
            };
            UpdatePortsList();

            btnRefresh = CreateBorderedButton("새로고침", 530, commY - 5, 120, 40); 
            btnRefresh.Click += (s, e) => UpdatePortsList();

            lblPortConnected = new Label { Text = "● 포트 연결됨", Location = new Point(680, commY), AutoSize = true, ForeColor = Color.Gray, Font = new Font("맑은 고딕", 11, FontStyle.Bold) }; 
            lblCommReceiving = new Label { Text = "● 통신 수신중", Location = new Point(880, commY), AutoSize = true, ForeColor = Color.Gray, Font = new Font("맑은 고딕", 11, FontStyle.Bold) }; 

            Label lblBaud = new Label { Text = "Baud Rate", Location = new Point(20, commY + 50), AutoSize = true, Font = new Font("Segoe UI", 11, FontStyle.Bold) };
            cmbBauds = new ComboBox
            {
                Location = new Point(160, commY + 48),
                Size = new Size(350, 35), 
                BackColor = Color.FromArgb(45, 45, 48),
                ForeColor = Color.FromArgb(0, 255, 200),
                DropDownStyle = ComboBoxStyle.DropDownList,
                Font = new Font("맑은 고딕", 11)
            };
            cmbBauds.Items.AddRange(new object[] { "9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600" });
            cmbBauds.SelectedItem = "115200";

            btnConnect = CreateBorderedButton("연결", 530, commY + 45, 100, 40); 
            btnConnect.Click += (s, e) => Connect();

            btnDisconnect = CreateBorderedButton("해제", 640, commY + 45, 100, 40); 
            btnDisconnect.Click += (s, e) => _protocol.Disconnect();

            btnInit = CreateBorderedButton("초기화", 750, commY + 45, 100, 40); 
            btnInit.Click += (s, e) => _protocol.ReInit();

            /* 프로토콜 선택: SCI / UDP */
            rdoSci = new RadioButton
            {
                Text = "SCI (기본)",
                Location = new Point(20, commY + 105),
                AutoSize = true,
                Checked = true,
                Font = new Font("맑은 고딕", 11, FontStyle.Bold),
                ForeColor = Color.Cyan
            };
            rdoUdp = new RadioButton
            {
                Text = "UDP (Ethernet)",
                Location = new Point(200, commY + 105),
                AutoSize = true,
                Font = new Font("맑은 고딕", 11, FontStyle.Bold),
                ForeColor = Color.Orange
            };
            rdoUdp.CheckedChanged += (s, e) =>
            {
                /* UDP 선택 시 COM Port/Baud 콤보박스 비활성화 */
                cmbPorts.Enabled = rdoSci.Checked;
                cmbBauds.Enabled = rdoSci.Checked;
            };

            lblCommStatus = new Label
            {
                Text = "",
                Location = new Point(680, commY + 50),
                Size = new Size(250, 30),
                Font = new Font("맑은 고딕", 11, FontStyle.Bold),
                ForeColor = Color.Red
            };

            pnlComm.Controls.AddRange(new Control[] {
                lblPort, lblBaud, cmbPorts, cmbBauds, btnRefresh,
                btnConnect, btnDisconnect, btnInit,
                lblPortConnected, lblCommReceiving,
                rdoSci, rdoUdp, lblCommStatus
            });
            mainLayout.Controls.Add(pnlComm, 0, 0);


            // 2. Status Panel - Sequence Number & Board Temperature & CRC
            Panel pnlStatus = CreateStyledPanel("MCU STATUS MONITOR (수신 데이터)");
            pnlStatus.Dock = DockStyle.Fill;
            pnlStatus.Margin = new Padding(5);

            Label lblSeqTitle = new Label { Text = "Seq. Num. (실시간):", Location = new Point(30, 60), AutoSize = true, Font = new Font("맑은 고딕", 14, FontStyle.Bold), ForeColor = Color.Cyan };
            lblSeqNumber = new Label { Text = "0", Location = new Point(320, 60), Size = new Size(200, 35), Font = new Font("Consolas", 18, FontStyle.Bold), ForeColor = Color.White };

            Label lblTempTitle = new Label { Text = "DSP Temp. (온도):", Location = new Point(30, 110), AutoSize = true, Font = new Font("맑은 고딕", 14, FontStyle.Bold), ForeColor = Color.Orange };
            lblBoardTemp = new Label { Text = "0.0 °C", Location = new Point(320, 110), Size = new Size(200, 35), Font = new Font("Consolas", 18, FontStyle.Bold), ForeColor = Color.White };

            Label lblCrcTitle = new Label { Text = "CRC Error Count:", Location = new Point(550, 60), AutoSize = true, Font = new Font("맑은 고딕", 14, FontStyle.Bold), ForeColor = Color.Red };
            lblCrcErrors = new Label { Text = "0", Location = new Point(750, 60), Size = new Size(200, 35), Font = new Font("Consolas", 18, FontStyle.Bold), ForeColor = Color.White };

            pnlStatus.Controls.AddRange(new Control[] { lblSeqTitle, lblSeqNumber, lblTempTitle, lblBoardTemp, lblCrcTitle, lblCrcErrors });
            mainLayout.Controls.Add(pnlStatus, 0, 1);


            // 3. Control Panel - Sequence Number Input
            Panel pnlCtrl = CreateStyledPanel("MCU CONTROL (송신 데이터)");
            pnlCtrl.Dock = DockStyle.Fill;
            pnlCtrl.Margin = new Padding(5);

            Label lblInputSeq = new Label { Text = "수동 Seq. Num. 전송 (0~255):", Location = new Point(30, 60), AutoSize = true, Font = new Font("맑은 고딕", 12, FontStyle.Bold) };
            txtInputSeq = new TextBox { 
                Location = new Point(380, 58), 
                Width = 100, 
                BackColor = Color.FromArgb(60, 60, 60), 
                ForeColor = Color.White, 
                Text = "0", 
                BorderStyle = BorderStyle.FixedSingle, 
                Font = new Font("Consolas", 14) 
            };
            btnSendSeq = CreateBorderedButton("Send Sequence", 500, 50, 180, 45);
            btnSendSeq.BackColor = Color.MediumSpringGreen;
            btnSendSeq.ForeColor = Color.Black;
            btnSendSeq.Click += (s, e) => 
            {
                if (byte.TryParse(txtInputSeq.Text, out byte manualSeq))
                {
                    _ctrlDto.ManualSeqNum = manualSeq;
                    _protocol.SendControlMessage(_ctrlDto);
                }
                else
                {
                    MessageBox.Show("0에서 255 사이의 올바른 숫자를 입력하세요.", "입력 오류", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
            };

            pnlCtrl.Controls.AddRange(new Control[] { lblInputSeq, txtInputSeq, btnSendSeq });
            mainLayout.Controls.Add(pnlCtrl, 0, 2);


            // 4. Real-Time Log Panel
            Panel pnlLog = CreateStyledPanel("REAL-TIME LOG MONITOR");
            pnlLog.Dock = DockStyle.Fill;
            pnlLog.Margin = new Padding(5);

            lblLogRxInfo = new Label { Location = new Point(30, 55), AutoSize = true, Font = new Font("Consolas", 11, FontStyle.Bold), ForeColor = Color.Lime };
            lblLogTxInfo = new Label { Location = new Point(30, 115), AutoSize = true, Font = new Font("Consolas", 11, FontStyle.Bold), ForeColor = Color.Yellow };

            Button btnLogDetail = CreateBorderedButton("로그 상세 보기\n(6K History)", 0, 45, 180, 90);
            btnLogDetail.Anchor = AnchorStyles.Right | AnchorStyles.Top;
            btnLogDetail.Click += (s, e) =>
            {
                if (_logForm.IsDisposed) _logForm = new LogForm();
                _logForm.Show();
                _logForm.BringToFront();
            };

            pnlLog.Controls.Add(lblLogRxInfo);
            pnlLog.Controls.Add(lblLogTxInfo);
            pnlLog.Controls.Add(btnLogDetail);

            pnlLog.Resize += (s, e) => { btnLogDetail.Location = new Point(pnlLog.Width - 220, 45); };

            mainLayout.Controls.Add(pnlLog, 0, 3);

            this.Controls.Add(mainLayout);
            UpdateConnectButtons();
        }

        private void SetupMenu()
        {
            MenuStrip menuStrip = new MenuStrip
            {
                BackColor = Color.FromArgb(45, 45, 48),
                ForeColor = Color.White,
                Font = new Font("Segoe UI", 10),
                RenderMode = ToolStripRenderMode.System
            };

            ToolStripMenuItem fileMenu = new ToolStripMenuItem("File (&F)");
            fileMenu.ForeColor = Color.White;
            fileMenu.DropDown.BackColor = Color.FromArgb(45, 45, 48);
            fileMenu.DropDown.ForeColor = Color.White;

            ToolStripMenuItem exitItem = new ToolStripMenuItem("Exit (&X)", null, (s, e) => this.Close());
            exitItem.ForeColor = Color.White;
            fileMenu.DropDownItems.Add(exitItem);

            ToolStripMenuItem helpMenu = new ToolStripMenuItem("Help (&H)");
            helpMenu.ForeColor = Color.White;
            helpMenu.DropDown.BackColor = Color.FromArgb(45, 45, 48);
            helpMenu.DropDown.ForeColor = Color.White;

            ToolStripMenuItem aboutItem = new ToolStripMenuItem("About (&A)", null, (s, e) => {
                MessageBox.Show("TMDSCNCD28388D_T Monitoring & Dashboard\n\n" +
                                "Version: 1.1 (Simplified Protocol)\n" +
                                "Date: 2026. 06. 01.\n" +
                                "Developer: Kim Jeonghwan (Nexcom)", 
                                "About Program", 
                                MessageBoxButtons.OK, 
                                MessageBoxIcon.Information);
            });
            aboutItem.ForeColor = Color.White;
            helpMenu.DropDownItems.Add(aboutItem);

            menuStrip.Items.Add(fileMenu);
            menuStrip.Items.Add(helpMenu);

            this.MainMenuStrip = menuStrip;
            this.Controls.Add(menuStrip);
        }



        private void UpdatePortsList()
        {
            string selected = cmbPorts.SelectedItem as string;
            cmbPorts.Items.Clear();

            try
            {
                using (var searcher = new ManagementObjectSearcher("SELECT * FROM Win32_PnPEntity WHERE Caption LIKE '%(COM%)'"))
                {
                    var ports = searcher.Get().Cast<ManagementBaseObject>().ToList();
                    var list = new List<string>();
                    
                    foreach (var p in ports)
                    {
                        string caption = p["Caption"].ToString();
                        list.Add(caption);
                    }

                    list.Sort((a, b) =>
                    {
                        var matchA = Regex.Match(a, @"\(COM(\d+)\)");
                        var matchB = Regex.Match(b, @"\(COM(\d+)\)");
                        int aNum = matchA.Success ? int.Parse(matchA.Groups[1].Value) : 0;
                        int bNum = matchB.Success ? int.Parse(matchB.Groups[1].Value) : 0;
                        return aNum.CompareTo(bNum);
                    });

                    foreach (var item in list) cmbPorts.Items.Add(item);
                }
            }
            catch
            {
                cmbPorts.Items.AddRange(SerialPort.GetPortNames());
            }

            if (cmbPorts.Items.Count > 0)
            {
                if (selected != null && cmbPorts.Items.Contains(selected))
                    cmbPorts.SelectedItem = selected;
                else
                    cmbPorts.SelectedIndex = 0;
            }
        }

        private void Connect()
        {
            try
            {
                if (_protocol != null && _protocol.IsConnected) _protocol.Disconnect();

                /* 프로토콜 선택: SCI 또는 UDP */
                if (rdoUdp != null && rdoUdp.Checked)
                {
                    _protocol = new UdpEthProtocol();
                }
                else
                {
                    if (cmbPorts.SelectedItem == null || cmbBauds.SelectedItem == null) return;
                    _protocol = new SciPcProtocol();
                }

                SetupProtocolEvents();

                if (rdoUdp != null && rdoUdp.Checked)
                {
                    /* UDP: portName/baudRate 인자는 무시됨 (클래스 내부 고정 IP 사용) */
                    _protocol.Connect("UDP", 0);
                }
                else
                {
                    string rawSelection = cmbPorts.SelectedItem.ToString();
                    string portName = rawSelection;
                    var match = Regex.Match(rawSelection, @"\((COM\d+)\)");
                    if (match.Success) portName = match.Groups[1].Value;
                    _protocol.Connect(portName, int.Parse(cmbBauds.SelectedItem.ToString()));
                }

                UpdateConnectButtons();
                _timer.Start();
            }
            catch (Exception ex)
            {
                MessageBox.Show("Connection Failed: " + ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void SetupProtocolEvents()
        {
            _protocol.OnStatusReceived += OnStatusReceived;
            _protocol.OnCommError += OnCommError;
            _protocol.OnPortClosed += () => { if (!IsDisposed) Invoke((Action)UpdateConnectButtons); };
            _protocol.OnRawTx += OnRawTxReceived;
            _protocol.OnRawRx += OnRawRxReceived;

            if (_protocol is SciPcProtocol sciProtocol)
            {
                sciProtocol.OnCrcErrorCountUpdated += OnCrcErrorCountUpdated;
            }
        }

        private void UpdateConnectButtons()
        {
            bool isConn = _protocol.IsConnected;
            btnConnect.Enabled = !isConn;
            btnDisconnect.Enabled = isConn;
            btnInit.Enabled = isConn;

            if (isConn)
            {
                lblPortConnected.ForeColor = Color.Lime;
            }
            else
            {
                lblPortConnected.ForeColor = Color.Gray;
                lblCommReceiving.ForeColor = Color.Gray;
                _timer.Stop();
            }
        }

        private void OnCrcErrorCountUpdated(int count)
        {
            BeginInvoke((Action)(() =>
            {
                lblCrcErrors.Text = count.ToString();
            }));
        }

        private void OnStatusReceived(StatusMessageData data)
        {
            BeginInvoke((Action)(() =>
            {
                _lastRxTime = DateTime.Now;
                lblCommReceiving.ForeColor = Color.Lime;

                lblSeqNumber.Text = data.IncNumber.ToString();
                lblBoardTemp.Text = string.Format("{0:0.0} °C", data.DspTemp);

                /* 통신 정상 수신 시 통신 두절 표시 해제 */
                if (lblCommStatus != null)
                    lblCommStatus.Text = "";
            }));
        }

        private void OnCommError(string msg)
        {
            if (_logForm != null && !_logForm.IsDisposed)
            {
                _logForm.AddLog($"[ERROR] {msg}");
            }

            /* 통신 두절 표시 (슈드 UI 스레드 안전 처리) */
            if (lblCommStatus != null && !IsDisposed)
            {
                BeginInvoke((Action)(() =>
                {
                    lblCommStatus.Text = "⚠ " + msg;
                    lblCommReceiving.ForeColor = Color.Red;
                }));
            }
        }

        private DateTime _lastRxLogTime = DateTime.MinValue;

        private void OnRawRxReceived(byte[] packet)
        {
            // 초당 500번(2ms 주기) 들어오는 Reflect 패킷으로 인한 C# UI 뻗음(Freeze) 완벽 방지
            // 100ms 이하 간격의 원시 로그는 UI에 그리지 않고 스킵합니다. (데이터 처리는 정상 동작함)
            if ((DateTime.Now - _lastRxLogTime).TotalMilliseconds < 100) return;
            _lastRxLogTime = DateTime.Now;

            BeginInvoke((Action)(() =>
            {
                string hex = BitConverter.ToString(packet).Replace("-", " ");
                lblLogRxInfo.Text = $"RX: {hex}";
                if (_logForm != null && !_logForm.IsDisposed) _logForm.AddLog($"RX: {hex}");
            }));
        }

        private void OnRawTxReceived(byte[] packet)
        {
            BeginInvoke((Action)(() =>
            {
                string hex = BitConverter.ToString(packet).Replace("-", " ");
                lblLogTxInfo.Text = $"TX: {hex}";
                if (_logForm != null && !_logForm.IsDisposed) _logForm.AddLog($"TX: {hex}");
            }));
        }

        private void Timer_Tick(object sender, EventArgs e)
        {
            // 통신 수신 인디케이터 관리 (500ms 무응답 시 회색)
            if ((DateTime.Now - _lastRxTime).TotalMilliseconds > 500)
            {
                lblCommReceiving.ForeColor = Color.Gray;
            }
        }

        // Custom UI Control Helpers
        private Panel CreateStyledPanel(string title)
        {
            Panel p = new Panel { BackColor = colorPanelBg, BorderStyle = BorderStyle.FixedSingle };
            Label l = new Label
            {
                Text = title,
                ForeColor = colorAccent,
                Font = new Font("Segoe UI", 12, FontStyle.Bold),
                Dock = DockStyle.Top,
                Height = 35,
                TextAlign = ContentAlignment.MiddleLeft,
                Padding = new Padding(10, 0, 0, 0),
                BackColor = Color.FromArgb(35, 35, 38)
            };
            p.Controls.Add(l);
            return p;
        }

        private Button CreateBorderedButton(string text, int x, int y, int w, int h)
        {
            Button btn = new Button
            {
                Text = text,
                Location = new Point(x, y),
                Size = new Size(w, h),
                FlatStyle = FlatStyle.Flat,
                Font = new Font("맑은 고딕", 10, FontStyle.Bold),
                BackColor = Color.FromArgb(45, 45, 48),
                ForeColor = Color.White,
                Cursor = Cursors.Hand
            };
            btn.FlatAppearance.BorderColor = colorAccent;
            btn.FlatAppearance.BorderSize = 1;
            btn.FlatAppearance.MouseOverBackColor = Color.FromArgb(60, 60, 65);
            return btn;
        }
    }
}
