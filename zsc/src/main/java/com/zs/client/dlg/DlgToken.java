package com.zs.client.dlg;

import java.awt.Dimension;
import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JButton;
import javax.swing.JDialog;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.JTextField;
import javax.swing.SpringLayout;

import com.zs.client.Utils;
import com.zs.client.client.ZsClient;

public class DlgToken extends JDialog {

	private static final long serialVersionUID = 1L;
	private JTextField txtTimeOut = new JTextField(10);
	private JButton btnNewTokenFile = new JButton("New token file");
	private JButton btnParseTokenFile = new JButton("Parse token file");
	private JButton btnGetToken = new JButton("Get token");
	private JButton btnNewToken = new JButton("New token");
	private JButton btnDeleteToken = new JButton("Delete token");
	
	private JTextArea txtText = new JTextArea();
	private ZsClient client = null;

	public DlgToken(ZsClient client) {
		
		this.client = client;
		
		if(!client.isNumberOpen()) {
			btnGetToken.setEnabled(false);
			btnNewToken.setEnabled(false);
			btnDeleteToken.setEnabled(false);
		}
		createGUI();
	}
	
	public void showDialog() {

		setModal(true);
		setMinimumSize(new Dimension(300, 200));
		setResizable(true);
		setTitle("Token");
				
		pack();
		setSize(550, 550);				
		setLocationRelativeTo(null);

		setVisible(true);
	}
	
	private JPanel createButtonPanel() {
		
		JPanel panel = new JPanel();
		GridLayout layout = new GridLayout(9, 1);
		panel.setLayout(layout);
		
		txtTimeOut.setText("10");
		panel.add(txtTimeOut);
		panel.add(btnNewTokenFile);
		panel.add(btnParseTokenFile);
		panel.add(new JLabel(""));
		panel.add(btnGetToken);
		panel.add(new JLabel(""));
		panel.add(btnNewToken);
		panel.add(new JLabel(""));
		panel.add(btnDeleteToken);
		return panel;
	}

	private void createGUI() {
		
		JPanel panel = new JPanel();
		SpringLayout layout = new SpringLayout();
		panel.setLayout(layout);
		
		JPanel panelButtons = createButtonPanel();
		JScrollPane panelText = new JScrollPane(txtText);		
		panel.add(panelButtons);
		panel.add(panelText);

		layout.putConstraint(SpringLayout.NORTH, panelText, 5, SpringLayout.NORTH, panel);		
		layout.putConstraint(SpringLayout.WEST, panelText, 5, SpringLayout.WEST, panel);
		layout.putConstraint(SpringLayout.SOUTH, panelText, -5, SpringLayout.SOUTH, panel);
		layout.putConstraint(SpringLayout.EAST, panelText, -5, SpringLayout.WEST, panelButtons);
		
		layout.putConstraint(SpringLayout.NORTH, panelButtons, 5, SpringLayout.NORTH, panel);		
		layout.putConstraint(SpringLayout.EAST, panelButtons, -5, SpringLayout.EAST, panel);

		getContentPane().add(panel);
		
		btnNewTokenFile.addActionListener(new ActionListener() {
			@Override public void actionPerformed(ActionEvent arg0) {
				newFile();
			}
		});

		btnParseTokenFile.addActionListener(new ActionListener() {
			@Override public void actionPerformed(ActionEvent arg0) {
				parseFile();
			}
		});
		
		btnGetToken.addActionListener(new ActionListener() {
			@Override public void actionPerformed(ActionEvent arg0) {
				getToken();
			}
		});

		btnNewToken.addActionListener(new ActionListener() {
			@Override public void actionPerformed(ActionEvent arg0) {
				newToken();
			}
		});

		btnDeleteToken.addActionListener(new ActionListener() {
			@Override public void actionPerformed(ActionEvent arg0) {
				deleteToken();
			}
		});		
	}
	
	private String getToken() {
		
		try {
			final String token = client.getNumToken();
			txtText.setText("Current token : " + token);
			return token;
		}
		catch(Exception e) {
			Utils.MessageBox_Error(e.getMessage());
		}
		return "";
	}	
	
	private void newToken() {
		
		int b = Utils.MessageBox_YESNO("Your current token will be replaced with new one.\nDo you want to create new number token ?");
		if(b == Utils.NO)
			return;
		
		try {
			final String token = client.newNumToken();
			txtText.setText("New token created : " + token);
		}
		catch(Exception e) {
			Utils.MessageBox_Error(e.getMessage());
		}
	}
	
	private void deleteToken() {
		
		int b = Utils.MessageBox_YESNO("Do you want to delete number token ?");
		if(b == Utils.NO)
			return;
		
		try {
			client.deleteNumToken();
			txtText.setText("Token deleted.");
		}
		catch(Exception e) {
			Utils.MessageBox_Error(e.getMessage());
		}
	}
	
	private String hideToken(final String token) {

		int len = token.length();
		if(len < 4)
			return "";

		String out = token.substring(0, 2);
		out += "[-- hidden value --]";
		out += token.substring(len - 2, len);
		return out;
	}
	
	private void newFile() {

		try {
			if(!client.isConnected())
				throw new Exception("Client not connected.");
			if(!client.isNumberOpen())
				throw new Exception("Number not open.");
			
			final String host = client.getHost();
			int port = client.getPort();
			int timeOutSec = 10;
			final String token = getToken();
			final String hiddenToken = hideToken(token);
			final String num = client.getOpenNumber();
			
			if(token.isEmpty())
				throw new Exception("Number has no token.");
			
			String userTimeOut = txtTimeOut.getText().trim();
			if(!userTimeOut.isEmpty()) {
				timeOutSec = Integer.parseInt(userTimeOut);
				if(timeOutSec < 1)
					timeOutSec = 10;
			}
				
			StringBuilder sb = new StringBuilder();
			sb.append("# Token file generated to send SMS only.\n");
			sb.append("# You can remove all lines, but 'Key'.\n\n");
			sb.append("# Server=").append(host).append("\n");
			sb.append("# Port=").append(port).append("\n");
			sb.append("# TimeOutSec=").append(timeOutSec).append("\n");
			sb.append("# Number=").append(num).append("\n");
			sb.append("# Token=").append(hiddenToken).append("\n");			
			
			final String encrToken = Utils.buildToken(host, port, timeOutSec, num, token);
			sb.append("Key=").append(encrToken).append("\n");
			appendText(sb.toString());
		}
		catch(Exception e) {
			Utils.MessageBox_Error(e.getMessage());
		}
	}

	private void parseFile() {

		try {
			final String encr = txtText.getText();
			final String[] lines = encr.split("\n");
			String key = "";
			for(String line : lines) {
				if(line.trim().startsWith("Key=")) {
					key = line;
					break;
				}
			}
			
			if(key.isEmpty())
				throw new Exception("'Key' not found.");
			
			final String[] pair = key.split("=");
			if(pair.length != 2)
				throw new Exception("Invalid key line.");
			
			final String decr = Utils.parseToken(pair[1]);
			final String[] params = decr.split(" ");
			if(params.length != 5)
				throw new Exception("Invalid params count.");
			
			StringBuilder sb = new StringBuilder();
			sb.append("Server=").append(params[0]).append("\n");
			sb.append("Port=").append(params[1]).append("\n");
			sb.append("Timeout=").append(params[2]).append("\n");
			sb.append("Number=").append(params[3]).append("\n");
			sb.append("Token=").append(params[4]).append("\n");
			
			appendText("\nParsed token file:\n");
			appendText(sb.toString());
		}
		catch(Exception e) {
			Utils.MessageBox_Error(e.getMessage());
		}
	}
	
	private void appendText(final String line) {
		
		txtText.setText(txtText.getText() + line);
	}
}
