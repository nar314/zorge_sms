package com.zs.client.dlg;

import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JButton;
import javax.swing.JDialog;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JTextField;
import javax.swing.SpringLayout;

import com.zs.client.Config;
import com.zs.client.Utils;
import com.zs.client.client.ComClientException;
import com.zs.client.client.ZsClient;

public class DlgConfig extends JDialog {

	private static final long serialVersionUID = 1L;

	public boolean OK = false;

	private Config config = null;
	private JTextField txtServer = new JTextField(20);
	private JTextField txtPort = new JTextField(5);
	private JTextField txtTimeout = new JTextField(7);

	private String server = "localhost";
	private int port = 7530;
	private int timeout = 10;
	
	public DlgConfig(final Config config) {
		
		OK = false;
		this.config = config;
		createGUI();
	}
	
	public void showDialog() {

		OK = false;
		if(config == null) {
			Utils.MessageBox_Error("Config not set");
			return;
		}

		setModal(true);
		setMinimumSize(new Dimension(350, 200));
		setResizable(true);
		setTitle("Config");
		
		txtServer.setText(config.getServer());
		txtPort.setText("" + config.getPort());
		txtTimeout.setText("" + config.getTimeout());
		
		pack();
		setSize(350, 200);
		setLocationRelativeTo(null);
		setVisible(true);		
	}

	private boolean checkValues() {
		
		server = txtServer.getText();
		if(server.isEmpty()) {
			txtServer.requestFocusInWindow();
			Utils.MessageBox_Error("Server is empty");
			return false;
		}

		final String portValue = txtPort.getText();
		try {
			port = Integer.parseInt(portValue);
		}
		catch(NumberFormatException e) {
			port = 0;
		}
		if(port < 1) {
			txtPort.requestFocusInWindow();
			Utils.MessageBox_Error("Port has invalid value");
			return false;
		}

		String timeoutValue = txtTimeout.getText();
		if(timeoutValue.isEmpty())
			timeoutValue = "-1"; // make it invalid
		try {
			timeout = Integer.parseInt(timeoutValue);
		}
		catch(NumberFormatException e) {
			timeout = 1;
		}
		if(timeout < 1) {
			txtTimeout.requestFocusInWindow();
			Utils.MessageBox_Error("Timeout has invalid value");
			return false;
		}
		
		return true;
	}
	
	private void pingServer() {
		
		try {
			ZsClient client = new ZsClient();
			client.connect(server, port, timeout);
			Utils.MessageBox_OK("Ping server OK.");			
		}
		catch(ComClientException e) {
			Utils.MessageBox_Error(e.getMessage());
		}
	}
	
	private JPanel createBottomButtons() {
		
		JPanel panel = new JPanel();

		JButton btnPing = new JButton("Ping server");
		btnPing.addActionListener(new ActionListener() {
			@Override public void actionPerformed(ActionEvent e) {
				if(!checkValues())
					return;				
				pingServer();
			}
		});
		
		JButton btnConnect = new JButton("Save and connect");
		btnConnect.addActionListener(new ActionListener() {
			@Override public void actionPerformed(ActionEvent e) {
				if(!checkValues())
					return;
				
				try {
					config.setServer(server);
					config.setPort(port);
					config.setTimeout(timeout);
				} 
				catch (Exception e1) {
					Utils.MessageBox_Error(e1.getMessage());
				}
				
				OK = true;
				dispose();
			}
		});
		
		JButton btnCancel = new JButton("Cancel");
		btnCancel.addActionListener(new ActionListener() {
			@Override public void actionPerformed(ActionEvent e) {
				OK = false;
				dispose();				
			}
		});
		
		panel.add(btnPing);
		panel.add(btnConnect);
		panel.add(btnCancel);
				
		return panel;
	}
	
	private void createGUI() {
		
		JPanel panel = new JPanel();
		SpringLayout layout = new SpringLayout();
		panel.setLayout(layout);

		JPanel panelServer = Utils.createPair(new JLabel("Server"), txtServer);
		JPanel panelPort = Utils.createPair(new JLabel("Port"), txtPort);
		JPanel panelTimeout = Utils.createPair(new JLabel("Timeout in seconds"), txtTimeout);
		JPanel panelButtons = createBottomButtons();
		
		panel.add(panelServer);
		panel.add(panelPort);
		panel.add(panelTimeout);
		panel.add(panelButtons);

		layout.putConstraint(SpringLayout.NORTH, panelServer, 5, SpringLayout.NORTH, panel);
		layout.putConstraint(SpringLayout.WEST, panelServer, 5, SpringLayout.WEST, panel);

		layout.putConstraint(SpringLayout.NORTH, panelPort, 5, SpringLayout.NORTH, panel);
		layout.putConstraint(SpringLayout.WEST, panelPort, 5, SpringLayout.EAST, panelServer);

		layout.putConstraint(SpringLayout.NORTH, panelTimeout, 5, SpringLayout.SOUTH, panelServer);
		layout.putConstraint(SpringLayout.WEST, panelTimeout, 5, SpringLayout.WEST, panel);
		
		layout.putConstraint(SpringLayout.SOUTH, panelButtons, -5, SpringLayout.SOUTH, panel);
		layout.putConstraint(SpringLayout.EAST, panelButtons, -5, SpringLayout.EAST, panel);
		
		getContentPane().add(panel);		
	}
}
