package com.zs.client.dlg;

import java.awt.Dimension;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JButton;
import javax.swing.JDialog;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JPasswordField;
import javax.swing.SpringLayout;

import com.zs.client.Utils;
import com.zs.client.client.ZsClient;

public class DlgChangePin extends JDialog {

	private static final long serialVersionUID = 1L;
	private ZsClient client = null;

	private JPasswordField txtCurPin = new JPasswordField(15);
	private JPasswordField txtNewPin = new JPasswordField(15);
	public Boolean OK = false;

	public DlgChangePin(ZsClient client) {
		
		OK = false;
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

	private JPanel createBottomButtons() {
		
		JPanel panel = new JPanel();
		JButton btnCreate = new JButton("Change pin");
		btnCreate.addActionListener(new ActionListener() {
			@Override public void actionPerformed(ActionEvent e) {

				DlgPsw dlg = new DlgPsw("Confirm new pin", "Confim new pin");
				dlg.showDialog();
				if(!dlg.OK)
					return; // User cancel

				final String newPin = new String(txtNewPin.getPassword());
				final String newPinConfirmed = dlg.psw;
				if(!newPinConfirmed.equals(newPin)) {
					Utils.MessageBox_Error("New pin not matched.");
					return;
				}
				
				final String curPin = new String(txtCurPin.getPassword());
				try {
					client.changePin(client.getOpenNumber(), curPin, newPin);
					OK = true;
					Utils.MessageBox_OK("Pin changed.");
				}
				catch(Exception e1) {
					Utils.MessageBox_Error(e1.getMessage());
					return;
				}
				dispose();
			}
		});
		
		JButton btnCancel = new JButton("Cancel");
		btnCancel.addActionListener(new ActionListener() {
			@Override public void actionPerformed(ActionEvent e) {
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

		JPanel panelCurPin = Utils.createPair(new JLabel("Current pin : "), txtCurPin);
		JPanel panelNewPin = Utils.createPair(new JLabel("New pin : "), txtNewPin);
		JPanel panelButtons = createBottomButtons();
		
		panel.add(panelCurPin);
		panel.add(panelNewPin);
		panel.add(panelButtons);
	
		layout.putConstraint(SpringLayout.NORTH, panelCurPin, 5, SpringLayout.NORTH, panel);		
		layout.putConstraint(SpringLayout.WEST, panelCurPin, 5, SpringLayout.WEST, panel);

		layout.putConstraint(SpringLayout.NORTH, panelNewPin, 5, SpringLayout.SOUTH, panelCurPin);		
		layout.putConstraint(SpringLayout.WEST, panelNewPin, 5, SpringLayout.WEST, panel);
		
		layout.putConstraint(SpringLayout.SOUTH, panelButtons, -5, SpringLayout.SOUTH, panel);
		layout.putConstraint(SpringLayout.EAST, panelButtons, -5, SpringLayout.EAST, panel);
		
		getContentPane().add(panel);
	}	
}
