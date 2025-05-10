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

public class DlgPsw extends JDialog {
	
	private static final long serialVersionUID = 1L;

	private String title = "";
	private String label = "";
	
	private JPasswordField txtPsw = new JPasswordField(20);
	private JLabel lblLabel = new JLabel("");
	
	public boolean OK = false;
	public String psw = "";
	
	public DlgPsw(final String title, final String label) {
		
		this.title = title;
		this.label = label;
		createGUI();
	}
	
	public void showDialog() {

		OK = false;
		psw = "";
		setModal(true);
		setMinimumSize(new Dimension(240, 120));
		setResizable(true);
		setTitle(title);
		
		lblLabel.setText(label);
				
		pack();
		setSize(240, 120);
		setLocationRelativeTo(null);
		setVisible(true);		
	}
	
	private JPanel createBottomButtons() {
		
		JPanel panel = new JPanel();

		JButton btnConnect = new JButton("OK");
		btnConnect.addActionListener(new ActionListener() {
			@Override public void actionPerformed(ActionEvent e) {
				psw = new String(txtPsw.getPassword());
				OK = true;
				dispose();
			}
		});
		
		JButton btnCancel = new JButton("Cancel");
		btnCancel.addActionListener(new ActionListener() {
			@Override public void actionPerformed(ActionEvent e) {
				OK = false;
				psw = "";
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

		JPanel panelPsw = Utils.createPair(lblLabel, txtPsw);
		JPanel panelButtons = createBottomButtons();
		
		panel.add(panelPsw);
		panel.add(panelButtons);

		layout.putConstraint(SpringLayout.NORTH, panelPsw, 5, SpringLayout.NORTH, panel);		
		layout.putConstraint(SpringLayout.WEST, panelPsw, 5, SpringLayout.WEST, panel);
		layout.putConstraint(SpringLayout.EAST, panelPsw, -5, SpringLayout.EAST, panel);
		
		layout.putConstraint(SpringLayout.SOUTH, panelButtons, -5, SpringLayout.SOUTH, panel);
		layout.putConstraint(SpringLayout.EAST, panelButtons, -5, SpringLayout.EAST, panel);
		
		getContentPane().add(panel);
	}
}
