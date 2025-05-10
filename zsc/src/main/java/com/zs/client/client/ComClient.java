package com.zs.client.client;

import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.net.UnknownHostException;
import java.util.Calendar;

/**
 * The communication client
 */
public class ComClient {

	static public short cmdHandShake = 1;
	static public short cmdConnect = 2;

	static public short cmdGetReply = 3;	
	static public short cmdGetDiag = 13;
	
	static public short cmdNumAdd = 50;
	static public short cmdNumDelete = 52;
	static public short cmdOpen = 56;
	static public short cmdChangePin = 57;
	
	static public short cmdTokenGet = 60;
	static public short cmdTokenNew = 61;
	static public short cmdTokenDelete = 62;
	static public short cmdTokenCheck = 63;
	
	static public short cmdMsgGetCount = 100;
	static public short cmdMsgSend = 101;
	static public short cmdMsgGetIds = 102;
	static public short cmdMsgRead = 103;
	static public short cmdMsgDelete = 104;
	
	static private int SocketTimeout = 1;
	static private int SocketOK = 2;
	
	static private String clientVersion = "1";
	
	private int port = 0;
	private String host = "";
	private InetAddress address = null;
	private DatagramSocket socket = null;
	
	private ComRequestBuilder rb = null;
	
	private byte keyId = 0;
	private String clientId = getNewGuid();
	private int seqNum = 0;
	private int seqNumStep = 1;	
	private int timeOutSec = 5; // 5 seconds
    private int maxResendTimes = 3; // re-send this much times

    private long timeOutEnd = 0L;
	final private int recvBufferLen = 1024 * 64;	
	final private byte[] recvBuffer = new byte[recvBufferLen];
	
	public boolean traceOn = false;
	
	static private char replyStatus_InProcess = '0';
	static private char replyStatus_Done = '1';
	static private char replyStatus_NotFound = '2';
	static private char replyStatus_Timeout = 'A';

	private class Reply {
		public char status = replyStatus_Timeout;
		public boolean isError = false;
		public String reply = "";
	};

	private String replyStatuToString(char ch) {
		
		switch(ch) {
		case '0' : return "In process";
		case '1' : return "Done";
		case '2' : return "Not found";
		case 'A' : return "Time out";
		case 'I' : return "Init";
		default: return "default";
		}
	}
	//-----------------------------------------------------------------------
	//
	//-----------------------------------------------------------------------
	public ComClient(final String host, int port) throws ComClientException {
		
		seqNum = (int)(Math.random() * 0xDEADBEEF + 1);
		seqNumStep = (int)(Math.random() * 0xDEAD + 1);
		
		rb = new ComRequestBuilder(clientId);
		this.port = port;
		this.host = host;		
		try {
	    	address = InetAddress.getByName(host);
	    	socket = new DatagramSocket();
		}
		catch(UnknownHostException | SocketException e) {
			throw new ComClientException(e.getMessage());
		}
    }	

	//-----------------------------------------------------------------------
	//
	//-----------------------------------------------------------------------	
	public void setTimeoutSec(int n) throws ComClientException {
		
		if(n <= 0)
			throw new ComClientException("Timeout has invalid value " + n);
		timeOutSec = n;
	}
	
	//-----------------------------------------------------------------------
	//
	//-----------------------------------------------------------------------		
	public int getTimeoutSec() {
		return timeOutSec;
	}

	//-----------------------------------------------------------------------
	//
	//-----------------------------------------------------------------------			
	public int getPort() {
		return port;
	}

	//-----------------------------------------------------------------------
	//
	//-----------------------------------------------------------------------				
	public String getHost() {
		return host;
	}
	
	//-----------------------------------------------------------------------
	//
	//-----------------------------------------------------------------------			
	public int getMaxResendTimes() {
		return maxResendTimes;
	}
	
	//-----------------------------------------------------------------------
	//
	//-----------------------------------------------------------------------	
	public void setMaxResendTimes(int n) throws ComClientException {
		if(n < 0)
			throw new ComClientException("Max resend times has invalid value " + n);
		maxResendTimes = n;
	}

	//-----------------------------------------------------------------------
	//
	//-----------------------------------------------------------------------		
	public void setTrace(boolean b) {
		traceOn = b;
	}

	private int send(final DatagramPacket request) throws ComClientException {

    	try {
	    	socket.setSoTimeout(timeOutSec * 1000);    		
			socket.send(request);
		} 
    	catch (SocketTimeoutException e) {
    		return SocketTimeout;
		}
    	catch(Exception e) {
    		throw new ComClientException(e.getMessage());
    	}

		return SocketOK;
	}

	private int receive(final DatagramPacket response) throws ComClientException {

    	try {
	    	socket.setSoTimeout(timeOutSec * 1000);    		
			socket.receive(response);
	    	socket.setSoTimeout(0);			
		} 
    	catch (SocketTimeoutException e) {
    		return SocketTimeout;
		}
    	catch(Exception e) {
    		throw new ComClientException(e.getMessage());
    	}

		return SocketOK;
	}
	
	private long getTimeT() {
		return Calendar.getInstance().getTime().getTime();
	}
	
	private void checkTimeOut() throws ComClientException {
		
		long curTimeT = Calendar.getInstance().getTime().getTime();
		if(traceOn)
			System.out.println("\tTimeout left=" + (timeOutEnd - curTimeT));
		if(curTimeT >= timeOutEnd)
			throw new ComClientException("Time out.");
	}
	
	private void sleep(long ms) {
		
		try {
			Thread.sleep(ms);
		} 
		catch (InterruptedException e) {
			System.out.println("sleep() failed.");
		}
	}
	
	public void Send(short cmdId, final String data) throws ComClientException  {
	
		for(int i = 0; i < maxResendTimes; ++i) {
			if(SendCmd(cmdId, data))
				return;
		}
		throw new ComClientException("Time out.");
	}
	
	private boolean SendCmd(short cmdId, final String data) throws ComClientException  {
		
		if(address == null || socket == null)
			throw new ComClientException("Not connected.");

        if(keyId == 0 && (cmdId != ComClient.cmdHandShake && cmdId != ComClient.cmdConnect)) 
        	throw new ComClientException("Invalid sequence of calls. Com client not connected.");

        if(cmdId == ComClient.cmdConnect)
        	seqNumStep = (int)(Math.random() * 0xDEAD + 1); // regenerate for each connect() because I can afford it.
        
        if(traceOn)
        	System.out.println("\tSending " + cmdId);
        
		seqNum += seqNumStep;
        byte[] buffer = rb.Create(seqNum, keyId, cmdId, data);
        
    	DatagramPacket request = new DatagramPacket(buffer, buffer.length, address, port);
        DatagramPacket response = new DatagramPacket(recvBuffer, recvBufferLen);
        
        // make call timeout as 3 x timeOutSec
        timeOutEnd = getTimeT() + maxResendTimes * timeOutSec * 1000;
        
       	int ns = send(request);
       	if(traceOn)
       		System.out.println("\t" + clientId + ", seq num=" + String.format("%X", seqNum));
       	if(ns == SocketTimeout)
       		throw new ComClientException("Send timeout."); // <- never see this happening.
       	
       	while(true) {
       		checkTimeOut();
       		if(traceOn)
       			System.out.println("\treceive()");
	        int nr = receive(response);
	        if(nr == SocketOK) {
	        	if(traceOn)
	        		System.out.println("\tparsing");
	        	rb.Parse(recvBuffer);
	        	
	        	// 1. Call sequence number has to match in the reply
	        	if(seqNum != rb.getReplySeqNum()) {
	        		if(traceOn)
	        			System.out.println("\tNot expected seq num. Expected " + String.format("%X", seqNum) + ", got " + String.format("%X", rb.getReplySeqNum()));
	        		continue; // skip wrong packet
	        	}
	        	
	        	if(cmdId != cmdHandShake && cmdId != cmdConnect) {
	        		// 2. Call key id has to match in the reply
	        		if(keyId != rb.getReplyKeyId()) {
	        			if(traceOn)
	        				System.out.println("\tNot expected keyId or client Id. Expected " + String.format("%X", keyId) + ", got " + String.format("%X", rb.getReplyKeyId()));
	        			continue; // skip wrong packet
	        		}
	        		// 3. Call client id has to match in the reply
	        		if(!clientId.equals(rb.getReplyClientId())) {
	        			if(traceOn)
	        				System.out.println("\tNot expected client Id. Expected " + clientId + ", got " + rb.getReplyClientId());
		        		continue; // skip wrong packet      			
	        		}
	        	}
	        	
	        	// This is reply for my request.
	        	break;
	        }	        
	        else if(nr == SocketTimeout) {
	        	if(cmdId == ComClient.cmdHandShake || cmdId == ComClient.cmdConnect)
	        		throw new ComClientException("Time out.");
	        	
	        	Reply r1 = null;
	        	while(true) {
	        		checkTimeOut();
	        		if(traceOn) 		
	        			System.out.println("\t----->Asking for reply");	        		
		        	r1 = getServerReply();
		        	
		        	char ch = r1.status;
		        	if(ch == replyStatus_InProcess || ch == replyStatus_Timeout) {
		        		if(traceOn)
		        			System.out.println("\tGot 'in process' or 'timeout'. Keep asking for reply.->" + replyStatuToString(r1.status));
		        		sleep(500);
		        		continue;
		        	}
		        	break; // for 'Done' or 'NotFound'
	        	}

	        	if(r1.status == replyStatus_NotFound)
	        		return false; // send command again
	        	
	        	if(r1.status != replyStatus_Done)
	        		throw new ComClientException("Assert(true)");
	        	
	        	if(traceOn) {
		        	System.out.println("\t------> reply");
		        	System.out.println("\tstatus = " + r1.status + "->" + replyStatuToString(r1.status));
		        	System.out.println("\tisError = " + r1.isError);
		        	System.out.println("\treply = " + r1.reply);
		        	System.out.println("\t------< reply");
	        	}
	        	break; // Got status 'Done'
	        }
       	}
       	       	
        if(rb.isReplyError())
        	throw new ComClientException(rb.getReplyAsString());
        
		return true;
	}
	
	private Reply getServerReply() throws ComClientException {

		if(address == null || socket == null)
			throw new ComClientException("Not connected.");

		if(traceOn)
			System.out.println("\tgetServerReply() seqNum=" + String.format("%X", seqNum) + ", clientId=" + clientId);
		byte[] buffer = rb.Create(seqNum, keyId, cmdGetReply, "");
    	DatagramPacket request = new DatagramPacket(buffer, buffer.length, address, port);
        DatagramPacket response = new DatagramPacket(recvBuffer, recvBufferLen);
        
       	int ns = send(request);
       	if(ns == SocketTimeout)
       		throw new ComClientException("Send timeout."); // <- never see this happening.
       	
       	while(true) {
       		checkTimeOut();
       		if(traceOn)
       			System.out.println("\tgetReply() calling receive()");
	        int nr = receive(response);
	        if(nr == SocketOK) {
	        	if(traceOn)	        	
	        		System.out.println("\tgetReply() parsing buffer");
	        	rb.Parse(recvBuffer);
	        	
	        	// 1. Call sequence number has to match in the reply
	        	if(seqNum != rb.getReplySeqNum()) {
	        		if(traceOn)
	        			System.out.println("\tNot expected seq num(1). Expected " + String.format("%X", seqNum) + ", got " + String.format("%X", rb.getReplySeqNum()));
	        		continue; // skip wrong packet
	        	}
	        	
        		// 2. Call key id has to match in the reply
        		if(keyId != rb.getReplyKeyId()) {
        			if(traceOn)
        				System.out.println("\tNot expected keyId or client Id(1). Expected " + String.format("%X", keyId) + ", got " + String.format("%X", rb.getReplyKeyId()));
        			continue; // skip wrong packet
        		}
        		// 3. Call client id has to match in the reply
        		if(!clientId.equals(rb.getReplyClientId())) {
        			if(traceOn)
        				System.out.println("\tNot expected client Id(1). Expected " + clientId + ", got " + rb.getReplyClientId());
	        		continue; // skip wrong packet
        		}
	        	
	        	// This is reply for my request.
        		if(traceOn)
        			System.out.println("\t---> getReply() this is my reply");
	        	break;
	        }	        
	        else if(nr == SocketTimeout) {
	        	Reply r = new Reply();
	        	r.status = replyStatus_Timeout;
	        	if(traceOn)
	        		System.out.println("\t---> getReply() I got timeout out");
	        	return r;
	        }
       	}

    	Reply r = new Reply();
    	r.status = rb.getReplyStatus();
    	r.isError = rb.isReplyError();
    	r.reply = rb.getReplyAsString();
    	
    	return r;
	}
	
	//-----------------------------------------------------------------------
	//
	//-----------------------------------------------------------------------		
	public String getReplyString() throws ComClientException {
		
		if(rb == null)
			throw new ComClientException("Assert rb == 0");
		try {
			return rb.getReplyAsString();
		}
		catch(Exception e) {
			throw new ComClientException(e.getMessage());
		}
	}
	
	private String getNewGuid() {
		
		String s = java.util.UUID.randomUUID().toString();
		StringBuilder sb = new StringBuilder();
		char ch = 0;
		for(int i = 0; i < s.length(); ++i) {
			ch = s.charAt(i);
			if(ch != '-')
				sb.append(ch);
		}
		return sb.toString();
	}
	
	private int hexToInt(char ch) throws Exception {
		
	    if (ch >= '0' && ch <= '9')
	        return ch - '0';
	    
	    if (ch >= 'A' && ch <= 'F')
	        return ch - 'A' + 10;
	    
	    if (ch >= 'a' && ch <= 'f')
	        return ch - 'a' + 10;
	    
	    throw new Exception("Invalid hex char : " + ch);
	}
	
	private byte[] hexStringToByteArray(final String input) throws Exception {
		
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
	
	//-----------------------------------------------------------------------
	//
	//-----------------------------------------------------------------------			
	public void Connect() throws ComClientException {
				
		SendCmd(cmdHandShake, clientVersion);		
		try {
			final String publicKey = getReplyString();
			final String guid = java.util.UUID.randomUUID().toString();			
			final String encrGuid = CoderRSA.Encrypt(guid, publicKey);
			SendCmd(cmdConnect, encrGuid);
			
			final String encrSessionKey = getReplyString();
			Coder c = new Coder();
			c.setKey(guid);
			
			byte[] bytesDecrSessionKey = c.decrypt(hexStringToByteArray(encrSessionKey));
			final String decrSessionKey = new String(bytesDecrSessionKey);
			
			String[] tokens = decrSessionKey.split(" ");
			if(tokens.length != 2) 
				throw new Exception("Connect. Invalid tokens size for session key");
			
			keyId = hexStringToByteArray(tokens[0])[0];
			rb.setKey(tokens[1]);			
		}
		catch(Exception e) {
			throw new ComClientException("Connect. " + e.getMessage());
		}
	}
}
