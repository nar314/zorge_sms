package com.zs.client;

import java.awt.Component;
import java.awt.Font;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.util.ArrayList;

import javax.swing.JFrame;
import javax.swing.JMenu;
import javax.swing.JMenuBar;
import javax.swing.JMenuItem;
import javax.swing.JPopupMenu;
import javax.swing.JScrollPane;
import javax.swing.JTable;
import javax.swing.table.DefaultTableCellRenderer;
import javax.swing.table.DefaultTableModel;
import javax.swing.table.TableColumnModel;

import com.zs.client.client.ComClientException;
import com.zs.client.client.ZsClient;
import com.zs.client.client.ZsClient.Message;
import com.zs.client.client.ZsClient.MessageCount;
import com.zs.client.client.ZsClient.MessageId;
import com.zs.client.client.ZsClient.MsgType;
import com.zs.client.dlg.DlgChangePin;
import com.zs.client.dlg.DlgConfig;
import com.zs.client.dlg.DlgCurStatus;
import com.zs.client.dlg.DlgMessage;
import com.zs.client.dlg.DlgNewNumber;
import com.zs.client.dlg.DlgOpenNumber;
import com.zs.client.dlg.DlgToken;

public class MainWindow {

	private static String title = "Zorge telegram";
	private JFrame frame = new JFrame(title);
	private MenuHolder menuHolder = new MenuHolder();
	private ZsClient client = null;
	private ConfigLoader configLoader = null;
	private Config config = null;
	private Thread thread = null;
	private Boolean threadExit = false;
	private Boolean refreshNow = false;
	
	private int COL_STATUS = 0;
	private int COL_FROM = 1;	
	private int COL_DATE = 2;
	private int COL_ID = 3;
	private String[] columns = new String[] {"", "", "", ""};
	private JTable table = new JTable(new Object[][]{}, columns);
	private Object dtmLock = new Object();
	
	private DefaultTableModel dtm = new DefaultTableModel(0, 4) {
		private static final long serialVersionUID = 1L;

		public boolean isCellEditable(int row, int column) {
			return false;
		}
		
		@Override public Class<?> getColumnClass(int column) {
           if (getRowCount() > 0) {
              Object value = getValueAt(0, column);
              if (value != null)
                 return getValueAt(0, column).getClass();
           }
           return super.getColumnClass(column);
        }
	};
	
	private class CellRenderer extends DefaultTableCellRenderer {

		private static final long serialVersionUID = 1L;
		@Override
		public Component getTableCellRendererComponent(JTable table, Object value, boolean isSelected, boolean hasFocus, int row, int column) {
			
		    super.getTableCellRendererComponent(table, value, isSelected, hasFocus, row, column);
		    final String s = (String)table.getValueAt(row, COL_STATUS);
		    if(s.equals("New"))
		    	this.setFont(this.getFont().deriveFont(Font.BOLD));
		    return this;
		}
	};

	//------------------------------------------------------------------
	//
	//------------------------------------------------------------------
	public MainWindow() {
		// Add hook for closing frame
		Runtime.getRuntime().addShutdownHook(new Thread() {
		    @Override public void run() {
		    	stopThread();
		    }
		});			
	}

	private void changeTitle() {
		
		if(config == null) {
			frame.setTitle("Not connected.");
			return;
		}
		
		if(client == null) {
			frame.setTitle("Not connected.");
			return;
		}
		String title = "";
		if(client.isConnected())
			title = "Connected to server";
		
		final String openNum = client.getOpenNumber();
		if(openNum != null && !openNum.isEmpty())
			title += " as " + openNum;
		else 
			title += ".";
		frame.setTitle(title);
	}
	
	//------------------------------------------------------------------
	//
	//------------------------------------------------------------------	
	public void show() throws Exception {

		createGUI();
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);		
		frame.pack();
		frame.setSize(700, 700);
		frame.setVisible(true);
		frame.setLocationRelativeTo(null);
		
		try {
			configLoader = new ConfigLoader();
			ArrayList<Config> configs = configLoader.loadConfigs();
			if(configs.size() == 0) {
				Config newConfig = new Config();
				DlgConfig dlg = new DlgConfig(newConfig);
				dlg.showDialog();
				if(!dlg.OK)
					return;
				
				configLoader.storeConfig(newConfig);
				configs.add(newConfig);
			}
			config = configs.get(0);
			if(config == null)
				throw new Exception("Config not found");
			
			client = new ZsClient();
			client.connect(config.getServer(), config.getPort(), config.getTimeout());
			client.setTrace(false);
			//frame.setTitle("Server connected. "+ config.getServer() + ":" + config.getPort());
			menuHolder.serverConnected(true);
			changeTitle();
			openNumberDlg(null, null);
		}
		catch(Exception e) {
			Utils.MessageBox_Error(e.getMessage());
		}
	}

	private void createGUI() {
		
        JScrollPane stp = new JScrollPane(table);
        table.setModel(dtm);
        table.setFillsViewportHeight(true);
        // make column with message id invisible in GUI.
        table.removeColumn(table.getColumnModel().getColumn(3));
		frame.getContentPane().add(stp);
		
		createMenu();
		setHeader();
		createPopupMenu();

		// Add handler for mouse double click
		table.addMouseListener(new MouseAdapter() {
			public void mousePressed(MouseEvent me) {
				JTable t = (JTable)me.getSource();
		        if (me.getClickCount() == 2 && t.getSelectedRow() != -1) {
		        	readMessage();
		        }
			}
		});
		
		table.getColumnModel().getColumn(COL_STATUS).setCellRenderer(new CellRenderer());
		table.getColumnModel().getColumn(COL_FROM).setCellRenderer(new CellRenderer());
		table.getColumnModel().getColumn(COL_DATE).setCellRenderer(new CellRenderer());
	}
	
	private void createPopupMenu() {
	
        JPopupMenu menu = new JPopupMenu();
        JMenuItem miOpen = new JMenuItem("Open message");
        JMenuItem miSend = new JMenuItem("Send new message");
        JMenuItem miDelete = new JMenuItem("Delete message");
        JMenuItem miRefresh = new JMenuItem("Refresh");
        
        miOpen.addActionListener(new ActionListener() {
			@Override public void actionPerformed(ActionEvent arg0) {
				readMessage();
			}
		});

        miSend.addActionListener(new ActionListener() {
			@Override public void actionPerformed(ActionEvent arg0) {
				sendMessage();
			}
		});
        
        miDelete.addActionListener(new ActionListener() {
			@Override public void actionPerformed(ActionEvent arg0) {
				deleteMessage();
			}
		});

        miRefresh.addActionListener(new ActionListener() {
			@Override public void actionPerformed(ActionEvent arg0) {
				refreshNow = true;
			}
		});
        
        menu.add(miOpen);
        menu.add(miSend);
        menu.add(miDelete);
        menu.addSeparator();
        menu.add(miRefresh);
        
        table.setComponentPopupMenu(menu);
	}
	
	private void createMenu() {
		
		JMenuBar menuBar = new JMenuBar();
		JMenu menuServer = new JMenu("Server");

		menuHolder.menuSrvConnect = new JMenuItem("Connect");
		menuHolder.menuSrvConnect.addActionListener(new ActionListener() {
			@Override public void actionPerformed(ActionEvent a) {
				connect();
			}
		});
		
		menuHolder.menuSrvDisconnect = new JMenuItem("Disconnect");
		menuHolder.menuSrvDisconnect.addActionListener(new ActionListener() {
			@Override public void actionPerformed(ActionEvent a) {
				disconnect();
			}
		});

		menuHolder.menuSrvEdit = new JMenuItem("Edit server settings.");
		menuHolder.menuSrvEdit.addActionListener(new ActionListener() {
			@Override public void actionPerformed(ActionEvent a) {
				editServer();
			}
		});
		
		menuHolder.menuSrvCurStatus = new JMenuItem("Current status");
		menuHolder.menuSrvCurStatus.addActionListener(new ActionListener() {
			@Override public void actionPerformed(ActionEvent a) {
				currentStatus();
			}
		});
		
		menuHolder.menuSrvExit = new JMenuItem("Exit");
		menuHolder.menuSrvExit.addActionListener(new ActionListener() {
			@Override public void actionPerformed(ActionEvent a) {
				frame.setVisible(false);
				frame.dispose();
			}
		});
		
		menuServer.add(menuHolder.menuSrvConnect);
		menuServer.add(menuHolder.menuSrvDisconnect);
		menuServer.add(menuHolder.menuSrvEdit);
		menuServer.addSeparator();
		menuServer.add(menuHolder.menuSrvCurStatus);
		menuServer.addSeparator();
		menuServer.add(menuHolder.menuSrvExit);
		
		JMenu menuNumber = new JMenu("Number");
		menuHolder.menuNumOpen = new JMenuItem("Open");
		menuHolder.menuNumOpen.addActionListener(new ActionListener() {
			@Override public void actionPerformed(ActionEvent a) {
				openNumberDlg(null, null);
			}
		});

		menuHolder.menuNumClose = new JMenuItem("Close");
		menuHolder.menuNumClose.addActionListener(new ActionListener() {
			@Override public void actionPerformed(ActionEvent a) {				
				closeNumber();
			}
		});

		menuHolder.menuNumAddNumber = new JMenuItem("Add number");
		menuHolder.menuNumAddNumber.addActionListener(new ActionListener() {
			@Override public void actionPerformed(ActionEvent a) {
				addNumber();
			}
		});
		
		menuHolder.menuNumToken = new JMenuItem("Token");
		menuHolder.menuNumToken.addActionListener(new ActionListener() {
			@Override public void actionPerformed(ActionEvent a) {
				DlgToken dlg = new DlgToken(client);
				dlg.showDialog();
			}
		});		

		menuHolder.menuNumChangePin = new JMenuItem("Change pin");
		menuHolder.menuNumChangePin.addActionListener(new ActionListener() {
			@Override public void actionPerformed(ActionEvent a) {
				DlgChangePin dlg = new DlgChangePin(client);
				dlg.showDialog();
				if(dlg.OK) {
					closeNumber();
					Utils.MessageBox_OK("Number is closed. Please open it again with new pin.");
				}
			}
		});		
		
		menuNumber.add(menuHolder.menuNumOpen);
		menuNumber.add(menuHolder.menuNumClose);
		menuNumber.addSeparator();	
		menuNumber.add(menuHolder.menuNumAddNumber);
		menuNumber.add(menuHolder.menuNumChangePin);
		menuNumber.add(menuHolder.menuNumToken);
		
		menuBar.add(menuServer);
		menuBar.add(menuNumber);
		frame.setJMenuBar(menuBar);
		
		menuHolder.serverConnected(false);
		changeTitle();
		menuHolder.numberOpen(false);
	}
	
	private void openNumberDlg(final String num, final String pin) {
		
		try {
			if(client == null)
				throw new Exception("Server not connected.");
			DlgOpenNumber dlg = new DlgOpenNumber(num, pin);
			dlg.showDialog();
			if(!dlg.OK)				
				return;
			
			synchronized(dtmLock) {
				stopThread();
				client.open(dlg.number, dlg.pin);
				menuHolder.numberOpen(true);
				changeTitle();
				startThread();
			}
		}
		catch(Exception e) {
			dtm.setRowCount(0);
			frame.setTitle(title);
			menuHolder.numberOpen(false);
			changeTitle();
			Utils.MessageBox_Error(e.getMessage());
		}
	}

	private void disconnect() {
		
		if(client == null)
			return;

		synchronized(dtmLock) {
			try {
				stopThread();
				client.disConnect();
				client = null;
				dtm.setRowCount(0);
				frame.setTitle(title);
			}
			catch(Exception e) {
				Utils.MessageBox_Error(e.getMessage());
			}
		}
		menuHolder.serverConnected(false);
		changeTitle();
	}
	
	private void setHeader() {

		TableColumnModel tcm = table.getColumnModel();
		tcm.getColumn(COL_STATUS).setHeaderValue("Status");
		tcm.getColumn(COL_FROM).setHeaderValue("From");		
		tcm.getColumn(COL_DATE).setHeaderValue("Date");	

		//tcm.getColumn(COL_STATUS).setWidth(30);
		//table.getTableHeader().repaint();
	}

	private void startThread() {
		
		thread = new Thread() {
			public void run() {
				threadFunc();
			}
		};
		threadExit = false;
		thread.start();
	}

	private void stopThread() {
		
		if(thread == null)
			return;
		
		threadExit = true;
		while(thread.isAlive())
			try {
				Thread.sleep(300);
			} 
			catch (InterruptedException e) { /* Nothing to do */ }	
	}
	
	private void threadFunc() {
		
		if(client == null)
			return;
		
		int timeOutSecs = config.getRefreshTimeout();
		int curNew = 0, curTotal = 0;
		MessageCount count = null;
		try {
			while(!threadExit) {
				count = client.getMsgCount();				
				//System.out.println("thread function. total = " + count.totalMsgs + ", new = " + count.newMsgs);
				if(curNew != count.newMsgs || curTotal != count.totalMsgs) {
					curNew = count.newMsgs;
					curTotal = count.totalMsgs;
					loadMessages();
				}
				
				refreshNow = false;
				for(int i = 0; threadExit == false && i < timeOutSecs * 2; ++i) {
					if(refreshNow)
						break;
					Thread.sleep(500);
				}
			}
			//System.out.println("thread is done.");
		}
		catch(Exception e) {
			System.out.println(e.getMessage());
		}
	}
	
	private void editServer() {
	
		try {
			DlgConfig dlg = new DlgConfig(config);
			dlg.showDialog();
			if(!dlg.OK)
				return;

			configLoader = new ConfigLoader();
			configLoader.storeConfig(config);
			
			disconnect();
			connect();
		}
		catch(Exception e) {
			Utils.MessageBox_Error(e.getMessage());
		}
	}
	
	//--------------------------------------------------------------------------
	// Connect to server, load all messages
	//--------------------------------------------------------------------------	
	private void connect() {
		
		try {
			stopThread();
			client = new ZsClient();
			client.connect(config.getServer(), config.getPort(), config.getTimeout());
			frame.setTitle("Server connected. "+ config.getServer() + ":" + config.getPort());
			// do not start thread here. Thread will be started when number is open.
			menuHolder.serverConnected(true);
			changeTitle();
			openNumberDlg(null, null);
		}
		catch(Exception e) {
			menuHolder.serverConnected(false);
			changeTitle();
			Utils.MessageBox_Error(e.getMessage());
		}
	}

	private void loadMessages() throws ComClientException {

		if(client == null)
			return;
		
		synchronized(dtmLock) {
			dtm.setRowCount(0);
			ArrayList<MessageId> msgs = client.getMsgIds(MsgType.All);
			//System.out.println("Total messages = " + msgs.size());
			for(MessageId m : msgs) {
				Object[] out = new Object[4];
				out[COL_STATUS] = m.status.trim();
				out[COL_DATE] = Utils.dateToString(m.dateLocal);
				out[COL_FROM] = m.fromNum;
				out[COL_ID] = m.id;
	
				dtm.addRow(out);			
			}
		}
	}
	
	private void readMessage() {

		if(client == null)
			return;

		try {
			Message m = null;
			synchronized(dtmLock) {
			int row = table.getSelectedRow();
			if(row == -1)
				return;
			String id = (String)dtm.getValueAt(row, COL_ID);
		
			m = client.readMsg(id);
			String status = (String)dtm.getValueAt(row, COL_STATUS);
			if(!status.equals("Read"))
				dtm.setValueAt("Read", row, COL_STATUS);
			}
			DlgMessage dlg = new DlgMessage(client, m, DlgMessage.Mode.Read);
			dlg.showDialog();
		}
		catch(Exception e) {
			Utils.MessageBox_Error(e.getMessage());
		}
	}
	
	private void sendMessage() {

		if(client == null)
			return;
		
		try {
			DlgMessage dlg = new DlgMessage(client, null, DlgMessage.Mode.New);			
			dlg.showDialog();
		}
		catch(Exception e) {
			Utils.MessageBox_Error(e.getMessage());
		}		
	}
	
	private void deleteMessage() {

		if(client == null)
			return;

		synchronized(dtmLock) {
			int row = table.getSelectedRow();
			if(row == -1)
				return;
			String id = (String)dtm.getValueAt(row, COL_ID);
			
			int n = Utils.MessageBox_YESNO("Do you want to delete selected message ? ");
			if(n == Utils.NO)
				return;
			
			try {
				client.deleteMsg(id);
				dtm.removeRow(row);
				loadMessages();
			}
			catch(Exception e) {
				Utils.MessageBox_Error(e.getMessage());
			}
		}
	}
	
	private void addNumber() {
		
		try {
			DlgNewNumber dlg = new DlgNewNumber(client);
			dlg.showDialog();
			if(dlg.OK) {
				int n = Utils.MessageBox_YESNO("Number created. Do you want to open it ? ");
				if(n == Utils.YES)
					openNumberDlg(dlg.newNum, dlg.newPin);
			}
		}
		catch(Exception e) {
			Utils.MessageBox_Error(e.getMessage());
		}		
	}
	
	private void currentStatus() {

		try {
			StringBuilder info = new StringBuilder();
			if(config == null)
				info.append("Config not set.\n"); 
			else {
				info.append("---- Config :\n");
				info.append("Name : ").append(config.getName()).append("\n");
				info.append("Server : ").append(config.getServer()).append("\n");
				info.append("Port : ").append(config.getPort()).append("\n");
				info.append("Timeout : ").append(config.getTimeout()).append("\n");				
			}
			
			boolean numOpen = false;
			if(client == null)
				info.append("\n\nClient not connected\n");
			else {
				info.append("\n---- Client :\n");
				info.append("Connected to server : ").append(client.isConnected()).append("\n");
				info.append("Is trace ON : ").append(client.isTraceOn()).append("\n");
				numOpen = client.isNumberOpen();
				info.append("Is number open : ").append(numOpen).append("\n");
				info.append("Open number : ").append(client.getOpenNumber() == null ? "" : client.getOpenNumber()).append("\n");
			}
			
			DlgCurStatus dlg = new DlgCurStatus(info.toString());
			dlg.showDialog();
		}
		catch(Exception e) {
			Utils.MessageBox_Error(e.getMessage());
		}		
	}
	
	private void closeNumber() {

		try {
			if(client == null)
				throw new Exception("Server not connected.");
			
			synchronized(dtmLock) {
				stopThread();
				client.close();
				menuHolder.numberOpen(false);
				dtm.setRowCount(0);
				frame.setTitle(title);						
			}
		}
		catch(Exception e) {					
			Utils.MessageBox_Error(e.getMessage());
		}
	}
}


