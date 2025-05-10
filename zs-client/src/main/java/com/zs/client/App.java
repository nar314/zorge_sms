package com.zs.client;

import java.io.BufferedReader;
import java.io.InputStreamReader;

import com.zs.client.client.ZsClient;

public class App {

    public static void main(String[] args) {
        
    	try {
    		ZsClient client = new ZsClient();  
    		TerminalExecutor.executeCommand("help", client);
    		
			while(true) {
				System.out.print(client.getPrompt() + "#");
				BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));
				String line = reader.readLine();
				
				if(line.equals("exit"))
					break;
				
				try {
					TerminalExecutor.executeCommand(line, client);
				}
				catch(Exception e) {
					System.out.println("Error : " + e.getMessage());
				}
			}
    	}
    	catch(Exception e) {
    		e.printStackTrace();
    		System.out.println("Error : " + e.getMessage());
    	}
    }
}
