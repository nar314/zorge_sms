package com.zs.client.dlg;

import java.awt.Dimension;

import javax.swing.JDialog;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.SpringLayout;

public class DlgCurStatus extends JDialog {
	
	private static final long serialVersionUID = 1L;
	private JTextArea text = new JTextArea();
	private String info = null;
	
	public DlgCurStatus(final String info) {
		
		this.info = info == null ? "" :info;
		createGUI();
	}
	
	public void showDialog() {
		
		setModal(true);
		setMinimumSize(new Dimension(360, 280));
		setResizable(true);
		setTitle("Current status.");
		
		text.setEditable(false);
		text.setText(info);	

		pack();
		setSize(260, 180);

		setLocationRelativeTo(null);
		setVisible(true);
	}

	private void createGUI() {
		
		JPanel panel = new JPanel();
		SpringLayout layout = new SpringLayout();
		panel.setLayout(layout);

		JScrollPane sp = new JScrollPane(text);
		panel.add(sp);

		layout.putConstraint(SpringLayout.NORTH, sp, 5, SpringLayout.NORTH, panel);		
		layout.putConstraint(SpringLayout.WEST, sp, 5, SpringLayout.WEST, panel);
		layout.putConstraint(SpringLayout.SOUTH, sp, -5, SpringLayout.SOUTH, panel);
		layout.putConstraint(SpringLayout.EAST, sp, -5, SpringLayout.EAST, panel);
		
		getContentPane().add(panel);
	}	
}
