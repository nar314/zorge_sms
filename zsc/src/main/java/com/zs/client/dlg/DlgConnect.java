package com.zs.client.dlg;

import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JButton;
import javax.swing.JDialog;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JPasswordField;
import javax.swing.JTextField;
import javax.swing.SpringLayout;

import com.zs.client.Utils;
import com.zs.client.client.ComClientException;
import com.zs.client.client.ZsClient;

public class DlgConnect extends JDialog {

	private static final long serialVersionUID = 1L;

	private JTextField txtServer = new JTextField(16);
	private JTextField txtPort = new JTextField(5);
	private JTextField txtNumber = new JTextField(15);
	private JPasswordField txtPin = new JPasswordField(15);
	private ZsClient client = null;
	
	public boolean OK = false;
	public String conString = "";
	
	public DlgConnect(ZsClient client) throws Exception {

		if(client == null)
			throw new Exception("Client not set");
		
		this.client = client;
		createGUI();
	}
	
	public void showDialog() {
		
		setModal(true);
		setMinimumSize(new Dimension(260, 260));
		setResizable(true);
		setTitle("Connect to server.");
		
		txtServer.setText("localhost");
		txtPort.setText("7530");
		
		pack();
		setSize(260, 260);
		setLocationRelativeTo(null);
		setVisible(true);
	}
	
	private boolean checkValue(JTextField obj, final String name, boolean numOnly) {
		
		String s = obj.getText();
		if(s.isEmpty()) {
			Utils.MessageBox_Error(name + " is empty.");
			obj.requestFocusInWindow();
			return false;
		}
		
		if(numOnly == true) {
			char[] values = s.toCharArray();
			for(char ch : values) {
				if(ch >= '0' && ch <= '9')
					continue;
				else {
					Utils.MessageBox_Error(name + " should contain only numbers. '" + ch + "' found.");
					obj.requestFocusInWindow();			
					return false;
				}
			}
		}
		return true;
	}
	
	private JPanel createBottomButtons() {
		JPanel panel = new JPanel();

		JButton btnConnect = new JButton("Connect");
		btnConnect.addActionListener(new ActionListener() {
			@Override public void actionPerformed(ActionEvent e) {
				if(!checkValue(txtServer, "Server", false))
					return;
				if(!checkValue(txtPort, "Port", true))
					return;
				if(!checkValue(txtNumber, "Number", true))
					return;
				if(!checkValue(txtPin, "Pin", false))
					return;

				try {
					final String server = txtServer.getText();
					int port = Integer.parseInt(txtPort.getText());
					
					client.connect(server, port);
					
					final String num = txtNumber.getText();
					final String pin = new String(txtPin.getPassword());
					
					client.open(num, pin);
					
					StringBuilder sb = new StringBuilder();
					sb.append(server).append(":").append(port).append(" ").append(num);
					conString = sb.toString();
				}
				catch(ComClientException er) {
					Utils.MessageBox_Error(er.getMessage());
					return;
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
		JPanel panelNumber = Utils.createPair(new JLabel("Number"), txtNumber);
		JPanel panelPin = Utils.createPair(new JLabel("Pin"), txtPin);
		JPanel panelButtons = createBottomButtons();
		
		panel.add(panelServer);
		panel.add(panelPort);
		panel.add(panelNumber);
		panel.add(panelPin);
		panel.add(panelButtons);

		layout.putConstraint(SpringLayout.NORTH, panelServer, 5, SpringLayout.NORTH, panel);		
		layout.putConstraint(SpringLayout.WEST, panelServer, 5, SpringLayout.WEST, panel);
		
		layout.putConstraint(SpringLayout.NORTH, panelPort, 5, SpringLayout.NORTH, panel);
		layout.putConstraint(SpringLayout.WEST, panelPort, 5, SpringLayout.EAST, panelServer);
		layout.putConstraint(SpringLayout.EAST, panelPort, -5, SpringLayout.EAST, panel);

		layout.putConstraint(SpringLayout.NORTH, panelNumber, 5, SpringLayout.SOUTH, panelServer);
		layout.putConstraint(SpringLayout.WEST, panelNumber, 5, SpringLayout.WEST, panel);
		layout.putConstraint(SpringLayout.EAST, panelNumber, -5, SpringLayout.EAST, panel);

		layout.putConstraint(SpringLayout.NORTH, panelPin, 5, SpringLayout.SOUTH, panelNumber);
		layout.putConstraint(SpringLayout.WEST, panelPin, 5, SpringLayout.WEST, panel);
		layout.putConstraint(SpringLayout.EAST, panelPin, -5, SpringLayout.EAST, panel);
		
		layout.putConstraint(SpringLayout.SOUTH, panelButtons, -5, SpringLayout.SOUTH, panel);
		layout.putConstraint(SpringLayout.EAST, panelButtons, -5, SpringLayout.EAST, panel);
		
		getContentPane().add(panel);
	}
}

