package com.zs.client;

import java.awt.Component;
import java.awt.GridLayout;
import java.io.BufferedReader;
import java.io.FileReader;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Random;

import javax.crypto.Cipher;
import javax.crypto.spec.IvParameterSpec;
import javax.crypto.spec.SecretKeySpec;
import javax.swing.JOptionPane;
import javax.swing.JPanel;

import com.zs.client.client.ZsClient;

public class Utils {

	//---------------------------------------------------------------------------
	//
	//---------------------------------------------------------------------------
	static public void MessageBox_OK(final String message) {
		JOptionPane.showMessageDialog(null, message, "Information", JOptionPane.INFORMATION_MESSAGE);
	}
	
	// 0 - Yes
	// 1 - No
	static public int YES = 0;
	static public int NO = 1;
	//---------------------------------------------------------------------------
	//
	//---------------------------------------------------------------------------	
	static public int MessageBox_YESNO(final String message) {
		return JOptionPane.showConfirmDialog(null, message, "Confirmation", JOptionPane.YES_NO_OPTION);
	}
	
	//---------------------------------------------------------------------------
	//
	//---------------------------------------------------------------------------	
	static public void MessageBox_Error(final String message) {	
		JOptionPane.showMessageDialog(null, message, "Error", JOptionPane.ERROR_MESSAGE);
	}
	
	static SimpleDateFormat df = new SimpleDateFormat("dd MMM yyyy HH:mm:ss");
	//---------------------------------------------------------------------------
	//
	//---------------------------------------------------------------------------	
	static public String dateToString(final Date d) {
		return df.format(d);
	}
	
	//---------------------------------------------------------------------------
	//
	//---------------------------------------------------------------------------	
	static public JPanel createPair(Component c1, Component c2) {
		
		JPanel panel = new JPanel();
		GridLayout layout = new GridLayout(2, 1);
		panel.setLayout(layout);
		panel.add(c1);
		panel.add(c2);
		return panel;
	}
	
	static private byte getByte() {

		Random rd = new Random();
	    byte[] arr = new byte[1];
	    rd.nextBytes(arr);
	    return arr[0];
	}
	
	static private String encryptToHex(final String input) throws Exception {
				
		byte[] plainBytes = input.getBytes();
		byte byteKey = getByte();
		
        byte[] iv = new byte[16];        
        for(int i = 0; i < 16; ++i, ++byteKey)
        	iv[i] = byteKey;
        byteKey -= 16;

        IvParameterSpec ivParameterSpec = new IvParameterSpec(iv);

        byte[] keyBytes = new byte[32];
        for(int i = 0; i < 16; ++i)
        	keyBytes[i] = iv[i];
        for(int i = 16; i < 32; ++i)
        	keyBytes[i] = iv[i - 16];
        
        SecretKeySpec secretKeySpec = new SecretKeySpec(keyBytes, "AES");
        Cipher cipher = Cipher.getInstance("AES/CBC/PKCS5Padding");
        cipher.init(Cipher.ENCRYPT_MODE, secretKeySpec, ivParameterSpec);
        byte[] encrypted = cipher.doFinal(plainBytes);
        
        StringBuffer sb = new StringBuffer();
        sb.append(String.format("%02X", byteKey));
    	for(int i = 0; i < encrypted.length; ++i)
    		sb.append(String.format("%02X", encrypted[i]));
        return sb.toString();
    }
	
	static private int hexToInt(char ch) throws Exception {
		
	    if (ch >= '0' && ch <= '9')
	        return ch - '0';
	    
	    if (ch >= 'A' && ch <= 'F')
	        return ch - 'A' + 10;
	    
	    if (ch >= 'a' && ch <= 'f')
	        return ch - 'a' + 10;
	    
	    throw new Exception("Invalid hex char : " + ch);
	}
	
	static private byte[] hexStringToByteArray(final String input) throws Exception {
		
		final int len = input.length();
		if(len % 2 != 0 || len == 0)
			throw new Exception("Invalid length");
		
		byte out[] = new byte[len / 2];
		for(int i = 0; i < len; i += 2) {
			int h = hexToInt(input.charAt(i));
			int l = hexToInt(input.charAt(i + 1));
			out[i / 2] = (byte) (h * 16 + l);
		}
		return out;
	}

	//---------------------------------------------------------------------------
	//
	//---------------------------------------------------------------------------
	static public String buildToken(final String server, int port, int timeoutSec, final String num, final String token) throws Exception {
		
		StringBuilder sb = new StringBuilder();
		sb.append(server).append(" ").append(port).append(" ").append(timeoutSec).append(" ").append(num).append(" ").append(token);
		return encryptToHex(sb.toString());
	}

	static private byte[] decrypt(final byte[] encryptedBytes) throws Exception {
		
		if(encryptedBytes == null)
			throw new Exception("Parameter is null.");
		
		byte byteKey = encryptedBytes[0];
        byte[] iv = new byte[16];        
        for(int i = 0; i < 16; ++i, ++byteKey)
        	iv[i] = byteKey;
        
        IvParameterSpec ivParameterSpec = new IvParameterSpec(iv);

        byte[] keyBytes = new byte[32];
        for(int i = 0; i < 16; ++i)
        	keyBytes[i] = iv[i];
        for(int i = 16; i < 32; ++i)
        	keyBytes[i] = iv[i - 16];

        int n = encryptedBytes.length - 1;
        byte[] toDecrypt = new byte[n];
        for(int i = 0; i < n; ++i)
        	toDecrypt[i] = encryptedBytes[i + 1];

        SecretKeySpec secretKeySpec = new SecretKeySpec(keyBytes, "AES");
        Cipher cipherDecrypt = Cipher.getInstance("AES/CBC/PKCS5Padding");
        cipherDecrypt.init(Cipher.DECRYPT_MODE, secretKeySpec, ivParameterSpec);
        byte[] decrypted = cipherDecrypt.doFinal(toDecrypt);
        return decrypted;
    }

	//---------------------------------------------------------------------------
	//
	//---------------------------------------------------------------------------	
	static public String parseToken(final String hexString) throws Exception {
		
		final byte[] encr = hexStringToByteArray(hexString);
		final byte[] ar = decrypt(encr);
		return new String(ar);
	}

	static private String fileToString(final String fileName) throws Exception {

		BufferedReader reader = new BufferedReader(new FileReader(fileName));
		String line = null;
		StringBuilder  sb = new StringBuilder();
		String ls = "\n";

	    try {
	        while((line = reader.readLine()) != null) {
	            sb.append(line);
	            sb.append(ls);
	        }

	        return sb.toString();
	    } 
	    finally {
	        reader.close();
	    }		
	}

	//---------------------------------------------------------------------------
	//
	//---------------------------------------------------------------------------		
	static public int sendMsg(final String[] args) throws Exception {
		// java -jar app.jar 202 token.file -m "This is the message"
		// java -jar app.jar 202 token.file -f message.txt

		String usage = "Sending message from the file: java -jar app.jar 202 token.file -f message.txt\n" +
						"Sending message from command line: java -jar app.jar 202 token.file -m \"Hello world!\"\n";
		
		int n = args.length;
		if(n != 4) {
			System.out.println("Invalid command line parameters.");
			System.out.println(usage);
			throw new Exception("Invalid command line parametes.");
		}
		
		final String numberTo = args[0];
		//
		// Load token and get parameters out of it.
		//
		System.out.println("Loading token file " + args[1]);
		final String tokenHex = fileToString(args[1]);
		System.out.println("Token file loaded.");
		if(tokenHex.isEmpty())
			throw new Exception("Token file is empty.");
		
		String key = null;
		final String[] lines = tokenHex.split("\n");
		for(String line : lines) {
			if(line.trim().startsWith("Key=")) {
				String[] pair = line.split("=");
				if(pair.length != 2)
					throw new Exception("Broken key value.");
				key = pair[1];
				break;
			}
		}
		if(key == null)
			throw new Exception("'Key' not found in the token.");
		
		System.out.println("Key decrypted.");
		
		final String[] params = parseToken(key).split(" ");
		//localhost 7530 10 101 CE82DD994DB44B0F921883FAC19205E1
		if(params.length != 5)
			throw new Exception("Invalid token content.");
		
		final String host = params[0];
		final String port = params[1];
		final String timeOut = params[2];
		final String numberFrom = params[3];
		final String pin = params[4];
		
		//
		// Load message.
		//
		final String mode = args[2];
		String message = "";
		if(mode.equals("-m")) {
			message = args[3];
		}
		else if(mode.equals("-f")) {
			System.out.println("Loading message from file "  + args[3]);
			message = fileToString(args[3]);
			System.out.println("Message loaded.");
		}
		else
			throw new Exception("Invalid command line parameter. Expected '-m' or '-f'");
		
		//
		// Send message
		//
		ZsClient client = new ZsClient();
		int timeOutNum = Integer.valueOf(timeOut);
		if(timeOutNum > 0)
			client.setTimeoutSec(timeOutNum);
		
		int portNum = Integer.valueOf(port);
		if(portNum < 1)
			throw new Exception("Invalid port.");
		
		System.out.println("Connecting to SMS server.");
		client.connect(host, portNum);
		System.out.println("SMS server connected.");
		System.out.println("Sending message to " + numberTo);
		client.sendMsgToken(numberFrom, numberTo, pin, message);		
		System.out.println("Message sent.");
		return 0;
	}
}
