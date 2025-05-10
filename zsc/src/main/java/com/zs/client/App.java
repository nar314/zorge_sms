package com.zs.client;

public class App {
	
    public static void main(String[] args) {
    	
    	Boolean guiMode = true;
    	try {
    		int rc = 0;    		
    		if(args.length == 0) {
	    		MainWindow w = new MainWindow();
	    		w.show();    			
    		}
    		else {
    			// java -jar app.jar 202 token.file -m "This is the message"
    			// java -jar app.jar 202 token.file -f message.txt
    			guiMode = false;
    			rc = Utils.sendMsg(args);
    			System.exit(rc);
    		}
    	}
    	catch(Exception e) {
    		if(guiMode)
    			Utils.MessageBox_Error(e.getMessage());
    		System.out.println(e.getMessage());
    		System.exit(1);
    	}
    }
}
