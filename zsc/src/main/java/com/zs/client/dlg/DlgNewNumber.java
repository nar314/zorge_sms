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
import com.zs.client.client.ZsClient;

public class DlgNewNumber extends JDialog {

	private static final long serialVersionUID = 1L;
	private ZsClient client = null;

	private JTextField txtNumber = new JTextField(15);
	private JPasswordField txtPin = new JPasswordField(15);
	public boolean OK = false;
	public String newNum = "";
	public String newPin = "";

	public DlgNewNumber(ZsClient client) throws Exception {
		
		if(client == null)
			throw new Exception("Client not set");
		
		this.client = client;
		createGUI();		
	}
	
	public void showDialog() {
		
		setModal(true);
		setMinimumSize(new Dimension(260, 180));
		setResizable(true);
		setTitle("Create new number.");
		
		pack();
		setSize(260, 180);
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

	private boolean createNum(final String num, final String pin) {

		try {
			client.addNumber(num, pin);
			return true;
		}
		catch(Exception e) {
			Utils.MessageBox_Error(e.getMessage());
			return false;
		}
	}
	
	private JPanel createBottomButtons() {
		JPanel panel = new JPanel();

		JButton btnCreate = new JButton("Create new number");
		btnCreate.addActionListener(new ActionListener() {
			@Override public void actionPerformed(ActionEvent e) {

				if(!checkValue(txtNumber, "Number", true))
					return; // Check failed
				if(!checkValue(txtPin, "Pin", false))
					return; // Check failed
				
				DlgPsw dlg = new DlgPsw("Confirm pin", "Confirm Pin");
				dlg.showDialog();
				if(!dlg.OK)
					return; // User cancel

				newNum = txtNumber.getText();
				newPin = new String(txtPin.getPassword());
				if(!dlg.psw.equals(newPin)) {
					Utils.MessageBox_Error("Pin not matched.");
					return; // Pin not matched
				}
				
				if(!createNum(newNum, newPin))
					return; // Error on server side.
				
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
		
		panel.add(btnCreate);
		panel.add(btnCancel);
				
		return panel;
	}

	private void createGUI() {
		
		JPanel panel = new JPanel();
		SpringLayout layout = new SpringLayout();
		panel.setLayout(layout);

		JPanel panelNumber = Utils.createPair(new JLabel("New number"), txtNumber);
		JPanel panelPin = Utils.createPair(new JLabel("Pin"), txtPin);
		JPanel panelButtons = createBottomButtons();
		
		panel.add(panelNumber);
		panel.add(panelPin);
		panel.add(panelButtons);
	
		layout.putConstraint(SpringLayout.NORTH, panelNumber, 5, SpringLayout.NORTH, panel);		
		layout.putConstraint(SpringLayout.WEST, panelNumber, 5, SpringLayout.WEST, panel);

		layout.putConstraint(SpringLayout.NORTH, panelPin, 5, SpringLayout.SOUTH, panelNumber);		
		layout.putConstraint(SpringLayout.WEST, panelPin, 5, SpringLayout.WEST, panel);
		
		layout.putConstraint(SpringLayout.SOUTH, panelButtons, -5, SpringLayout.SOUTH, panel);
		layout.putConstraint(SpringLayout.EAST, panelButtons, -5, SpringLayout.EAST, panel);
		
		getContentPane().add(panel);
	}
}
