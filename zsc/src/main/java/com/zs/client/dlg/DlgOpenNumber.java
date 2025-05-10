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

public class DlgOpenNumber extends JDialog {

	private static final long serialVersionUID = 1L;
	
	private JTextField txtNumber = new JTextField(15);
	private JPasswordField txtPin = new JPasswordField(15);
	
	public boolean OK = false;
	public String number = "";
	public String pin = "";
	
	public DlgOpenNumber(final String number, final String pin) {
		
		this.number = number;
		this.pin = pin;
		if(this.number == null)
			this.number = "";
		if(this.pin == null)
			this.pin = "";
		
		createGUI();
	}
	
	public void showDialog() {
		
		OK = false;
		setModal(true);
		setMinimumSize(new Dimension(200, 180));
		setResizable(true);
		setTitle("Open number");
		
		pack();
		setSize(200, 180);
		setLocationRelativeTo(null);
		setVisible(true);				
	}
	
	private boolean checkValues() {
		
		number = txtNumber.getText();
		if(number.isEmpty()) {
			txtNumber.requestFocusInWindow();
			Utils.MessageBox_Error("Number is empty");
			return false;
		}

		pin = new String(txtPin.getPassword());
		if(pin.isEmpty()) {
			txtPin.requestFocusInWindow();
			Utils.MessageBox_Error("Pin is empty");
			return false;
		}
		
		return true;
	}
	
	private JPanel createBottomButtons() {
		
		JPanel panel = new JPanel();

		JButton btnOK = new JButton("OK");
		btnOK.addActionListener(new ActionListener() {
			@Override public void actionPerformed(ActionEvent e) {
				if(!checkValues())
					return;
				
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
		
		panel.add(btnOK);
		panel.add(btnCancel);

		return panel;
	}
	
	public void createGUI() {
		
		JPanel panel = new JPanel();
		SpringLayout layout = new SpringLayout();
		panel.setLayout(layout);

		txtNumber.setText(number);
		txtPin.setText(pin);

		JPanel panelNumber = Utils.createPair(new JLabel("Number"), txtNumber);
		JPanel panelPin = Utils.createPair(new JLabel("Pin"), txtPin);
		JPanel panelButtons = createBottomButtons();
		
		panel.add(panelNumber);
		panel.add(panelPin);
		panel.add(panelButtons);

		layout.putConstraint(SpringLayout.NORTH, panelNumber, 5, SpringLayout.NORTH, panel);
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
