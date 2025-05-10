package com.zs.client;

import javax.swing.JMenuItem;

public class MenuHolder {

	// Server
	public JMenuItem menuSrvConnect = null;
	public JMenuItem menuSrvDisconnect = null;
	public JMenuItem menuSrvEdit = null;
	public JMenuItem menuSrvCurStatus = null;
	public JMenuItem menuSrvExit = null;
	
	// Number
	public JMenuItem menuNumOpen = null;
	public JMenuItem menuNumClose = null;
	public JMenuItem menuNumAddNumber = null;
	public JMenuItem menuNumToken = null;
	public JMenuItem menuNumChangePin = null;
	
	public void serverConnected(boolean connected) {

		if(connected) {
			menuSrvDisconnect.setEnabled(true);
			menuNumOpen.setEnabled(true);
			menuNumAddNumber.setEnabled(true);
			menuNumToken.setEnabled(true);
		}
		else {
			menuSrvDisconnect.setEnabled(false);
			menuNumOpen.setEnabled(false);
			menuNumClose.setEnabled(false);
			menuNumAddNumber.setEnabled(false);
			menuNumToken.setEnabled(false);
			menuNumChangePin.setEnabled(false);
		}
	}
	
	public void numberOpen(boolean open) {
		
		if(open) {
			menuNumClose.setEnabled(true);
			menuNumChangePin.setEnabled(true);
		}
		else {
			menuNumClose.setEnabled(false);
			menuNumChangePin.setEnabled(false);
		}			
	}
}
