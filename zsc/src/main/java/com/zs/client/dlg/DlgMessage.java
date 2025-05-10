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
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;

import com.zs.client.Utils;
import com.zs.client.client.ComClientException;
import com.zs.client.client.ZsClient;
import com.zs.client.client.ZsClient.Message;

public class DlgMessage extends JDialog {

	private static final long serialVersionUID = 1L;
	private ZsClient client = null;
	private Message message = null;
	private Mode mode = Mode.New;

	private JButton btnSendOrClose = new JButton();
	private JButton btnReply = new JButton("Reply");
	private JTextField txtDate = new JTextField(12);
	private JTextField txtNum = new JTextField(12);
	private JTextArea txtText = new JTextArea();
	
	public enum Mode {
		New, Read, Reply
	};

	//----------------------------------------------------------------	
	// Constructor for all modes
	//----------------------------------------------------------------	
	public DlgMessage(ZsClient client, final Message message, Mode mode) {
		
		this.client = client;
		this.message = message;
		this.mode = mode;
		
		createGUI();

		txtDate.setEditable(false);

		if(mode == Mode.New) {
			txtNum.setEditable(true);
			txtText.setEditable(true);
			
			btnSendOrClose.setText("Send");
			btnSendOrClose.setEnabled(false);		
			btnReply.setVisible(false);
		}
		else if(mode == Mode.Read) {
			txtNum.setEditable(false);
			txtText.setEditable(false);
			
			btnSendOrClose.setText("Close");
			btnSendOrClose.setEnabled(true);		
			btnReply.setVisible(true);
			
			txtDate.setText(Utils.dateToString(message.dateLocal));
			txtNum.setText(message.fromNum);
			txtText.setText(message.text);			
		}
		else if(mode == Mode.Reply) {
			txtNum.setEditable(false);
			txtText.setEditable(true);
			
			btnSendOrClose.setText("Send");
			btnSendOrClose.setEnabled(true);
			btnReply.setVisible(false);
			
			txtNum.setText(message.fromNum);
			txtText.setText(message.text);			
		}
		else
			System.out.println("What is that mode ?");
	}

	//----------------------------------------------------------------	
	// 
	//----------------------------------------------------------------	
	public void showDialog() {

		setModal(true);
		setMinimumSize(new Dimension(300, 200));
		setResizable(true);
		if(mode == Mode.Read)
			setTitle("Read message");
		else if(mode == Mode.New)
			setTitle("Send message");
		else if(mode == Mode.Reply)
			setTitle("Reply message");
		else
			setTitle("");
				
		pack();
		setSize(800, 550);				
		setLocationRelativeTo(null);

		if(mode == Mode.Read)
			txtText.requestFocusInWindow();
		else			
			txtNum.requestFocusInWindow();

		setVisible(true);
	}
	
	private JPanel createPanel() {
		
		JPanel panel = new JPanel();
		GridLayout layout = new GridLayout(2, 1);
		panel.setLayout(layout);
		
		JPanel pairD = Utils.createPair(new JLabel("Date :"), txtDate);
		
		final String lblText = mode == Mode.Read ? "From" : "To";
		JPanel pairF = Utils.createPair(new JLabel(lblText), txtNum);
		
		panel.add(pairD);
		panel.add(pairF);
/*		
		txtDate.setHorizontalAlignment(SwingConstants.LEFT);
		txtNum.setHorizontalAlignment(SwingConstants.LEFT);
		pairD.setAlignmentX(SwingConstants.LEFT);
		pairF.setAlignmentX(SwingConstants.LEFT);
*/
		return panel;
	}
	
	private void createGUI() {
		
		JPanel panel = new JPanel();
		SpringLayout layout = new SpringLayout();
		panel.setLayout(layout);
		
		JPanel panelNum = createPanel();
		panel.add(panelNum);
		panel.add(btnSendOrClose);
		panel.add(btnReply);
		JScrollPane panelText = new JScrollPane(txtText);
		panel.add(panelText);
		
		layout.putConstraint(SpringLayout.NORTH, panelNum, 5, SpringLayout.NORTH, panel);		
		layout.putConstraint(SpringLayout.WEST, panelNum, 5, SpringLayout.WEST, panel);
		
		layout.putConstraint(SpringLayout.NORTH, btnSendOrClose, 5, SpringLayout.NORTH, panel);		
		// Do not link to the WEST to keep original button size. 
		layout.putConstraint(SpringLayout.EAST, btnSendOrClose, -5, SpringLayout.EAST, panel);
		
		layout.putConstraint(SpringLayout.NORTH, btnReply, 5, SpringLayout.SOUTH, btnSendOrClose);
		layout.putConstraint(SpringLayout.EAST, btnReply, -5, SpringLayout.EAST, panel);
		
		layout.putConstraint(SpringLayout.NORTH, panelText, 5, SpringLayout.SOUTH, panelNum);
		layout.putConstraint(SpringLayout.WEST, panelText, 5, SpringLayout.WEST, panel);
		layout.putConstraint(SpringLayout.EAST, panelText, -5, SpringLayout.EAST, panel);
		layout.putConstraint(SpringLayout.SOUTH, panelText, -5, SpringLayout.SOUTH, panel);

		getContentPane().add(panel);
		
		btnSendOrClose.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent arg0) {
				if(client == null || mode == Mode.Read) {
					dispose();
					return;
				}				
				
				try {
					final String to = txtNum.getText();
					if(to.isEmpty())
						throw new ComClientException("\'To\' is empty.");
					client.sendMsg(to, txtText.getText());
					Utils.MessageBox_OK("Message sent");
					dispose();
				}
				catch(ComClientException e) {
					Utils.MessageBox_Error(e.getMessage());
					return;
				}
			}
		});
		
		btnReply.addActionListener(new ActionListener() {
			public void actionPerformed(ActionEvent arg0) {
				
				DlgMessage d = new DlgMessage(client, message, DlgMessage.Mode.Reply);
				d.showDialog();
				dispose();
			}			
		});
		
		txtText.getDocument().addDocumentListener(new DocumentListener() {		
			@Override public void removeUpdate(DocumentEvent arg0) {
				textChanged();
			}
			
			@Override public void insertUpdate(DocumentEvent arg0) {
				textChanged();
			}
			
			@Override public void changedUpdate(DocumentEvent arg0) {
				textChanged();
			}
		});
		
		if(client != null) {
			txtNum.getDocument().addDocumentListener(new DocumentListener() {		
				@Override public void removeUpdate(DocumentEvent arg0) {
					numChanged();
				}
				
				@Override public void insertUpdate(DocumentEvent arg0) {
					numChanged();
				}
				
				@Override public void changedUpdate(DocumentEvent arg0) {
					numChanged();
				}
			});
		}
	}
	
	private void numChanged() {
		if(client != null)
			btnSendOrClose.setEnabled(txtNum.getText().length() > 0);
	}
	
	private void textChanged() {
		
		final String s = txtText.getText();
		if(s.length() > 4096) {
			System.out.println("Message length " + s.length());
			Utils.MessageBox_Error("Message is too long. 4K is the limit.");
		}
	}
}
