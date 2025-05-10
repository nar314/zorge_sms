package com.zs.client.client;

import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.TimeZone;

public class ZsClient {

	private ComClient client = null;

	private boolean showPin = true;	
	private String openNum = null;
	private String openPin = null;

	private Boolean traceOn = false;
	private Integer preSetTimeOutSec = null;
	private int MAX_CALLS_IN_CONNECT = 3;
	
	private static String statusToString(final String status) {
		
		if(status.equals("N"))
			return "New ";
		if(status.equals("R"))
			return "Read";
		if(status.equals("D"))
			return "Del ";
		return "Unknown=" + status;		
	}
	
    private static Date getLocal(final String timetUtc) {

    	Long n = Long.parseLong(timetUtc) * 1000;
    	Calendar c = Calendar.getInstance();
    	c.setTimeInMillis(n);
    	
    	Date dateUtc = c.getTime();
        String timeZone = Calendar.getInstance().getTimeZone().getID();
        Date dateLocal = new Date(dateUtc.getTime() + TimeZone.getTimeZone(timeZone).getOffset(dateUtc.getTime()));
        return dateLocal;
    }

    public class MessageCount {
    	
    	public int totalMsgs = 0;
    	public int newMsgs = 0; 
    }
    
	public class MessageId {
		
		public String status = null;
		public Date dateLocal = null;
		public String fromNum = null;
		public String id = null;
		
		public MessageId(final String line) throws ComClientException {
			
			String tokens[] = line.split(" ");
			if(tokens.length != 4)
				throw new ComClientException("Invalid message id line : " + line);
			status = statusToString(tokens[0]);
			dateLocal = getLocal(tokens[1]);
			fromNum = tokens[2];
			id = tokens[3];
		}
	}
	
	public enum MsgType { New, All }
	
	public class Message {
		
		// This is a problem. When message contains separator, parsing will fail.
		// FIX ME Pass separator inside reply. Caller should check that message does not contains separator.
		// Yep, performance it is.
		private String separator = "]@@]";
		public String status = null;
		public Date dateLocal = null;
		public String fromNum = null;
		public String text = null;
		
		public Message(final String line) throws ComClientException {

			String tokens[] = line.split(separator);
			int len = tokens.length;
			if(len < 3)
				throw new ComClientException("Invalid message line : " + line);
			status = tokens[0];
			dateLocal = getLocal(tokens[1]);
			fromNum = tokens[2];
			if(len == 3)
				text = "";
			else {
				int allSepLen = 3 * separator.length();
				int n = tokens[0].length() + tokens[1].length() + tokens[2].length() + allSepLen;
				text = line.substring(n);
			}
		}
	}
	
	public ZsClient() throws ComClientException {

        if(traceOn)
        	System.out.println("TRACE : ZtClient() ");
	}
	
	synchronized public boolean isTraceOn() {
		return traceOn;
	}
	
	synchronized public void setTrace(boolean b) {
		
        if(traceOn)
        	System.out.println("TRACE : setTrace() ");
        traceOn = b;
        if(client != null)
        	client.traceOn = b;
	}
	
	synchronized public void connect(final String host, int port) throws ComClientException {
		
        if(traceOn)
        	System.out.println("TRACE : connect ");

        if(client == null) {
			client = new ComClient(host, port);
			client.setTrace(traceOn);
			if(preSetTimeOutSec != null)
				client.setTimeoutSec(preSetTimeOutSec);
        }
        
		openNum = openPin = null;
        int tries = MAX_CALLS_IN_CONNECT;
        while(tries > 0) {
	        try {
	        	client.Connect();
	        	break;
	        }
	        catch(ComClientException e) {
	        	if(e.getMessage().contains("Time out")) {
	        		tries--;
	        		if(tries == 0)
		        		throw e;
	        	}
	        	else
	        		throw e;
	        }
        }
	}

	synchronized public void connect(final String host, int port, int timeOutSec) throws ComClientException {

        if(traceOn)
        	System.out.println("TRACE : connect() ");

		openNum = openPin = null;        
		client = new ComClient(host, port);
		client.setTrace(traceOn);
		client.setTimeoutSec(timeOutSec);

		int tries = MAX_CALLS_IN_CONNECT;
        while(tries > 0) {
	        try {
	        	client.Connect();
	        	break;
	        }
	        catch(ComClientException e) {
	        	if(e.getMessage().contains("Time out")) {
	        		tries--;
	        		if(tries == 0)
	        			throw e;
	        	}
	        	else
	        		throw e;
	        }
        }		
	}

	synchronized public boolean isConnected() {
		return client != null;
	}
	
	synchronized public boolean isNumberOpen() {
		return openNum != null;
	}

	synchronized public String getOpenNumber() {
		return openNum;
	}

	synchronized public void disConnect() throws ComClientException {

        if(traceOn)
        	System.out.println("TRACE : disConnect() ");
		client = null;
		openNum = openPin = null;
	}
	
	// remove this method. Added for shell.
	public String getPrompt() {
		
		if(openNum == null)
			return "";
		
		StringBuilder sb = new StringBuilder().append(openNum);
		if(showPin)
			sb.append(" {").append(openPin).append("}");
		return sb.toString();
	}
	
	synchronized public void close() {
		
        if(traceOn)
        	System.out.println("TRACE : close() ");

		openNum = openPin = null;
	}
	
	synchronized public String addNumber(final String num, final String pin) throws ComClientException {
		
        if(traceOn)
        	System.out.println("TRACE : addNumber() ");

		if(client == null)
			throw new ComClientException("Not connected");
		if(num == null || pin == null)
			throw new ComClientException("Assert num == null || pin == null");
		if(num.length() == 0 || pin.length() == 0)
			throw new ComClientException("Assert num.len == 0 || pin.len == 0");
			
		StringBuilder sb = new StringBuilder().append(num).append("\n").append(pin);
		client.Send(ComClient.cmdNumAdd, sb.toString());
		return client.getReplyString();
	}

	synchronized public void deleteNumber(final String num, final String pin) throws ComClientException {
		
        if(traceOn)
        	System.out.println("TRACE : deleteNumber() ");

		if(client == null)
			throw new ComClientException("Not connected");
		if(num == null || pin == null)
			throw new ComClientException("Assert num == null || pin == null");
		if(num.length() == 0 || pin.length() == 0)
			throw new ComClientException("Assert num.len == 0 || pin.len == 0");
		
		StringBuilder sb = new StringBuilder().append(num).append("\n").append(pin);
		client.Send(ComClient.cmdNumDelete, sb.toString());
	}
	
	synchronized public ArrayList<MessageId> getMsgIds(MsgType msgType) throws ComClientException {
		
        if(traceOn)
        	System.out.println("TRACE : getMsgIds() ");

		if(client == null)
			throw new ComClientException("Not connected");
		if(openNum == null)
			throw new ComClientException("Number not open.");		
		
		String type = "1";
		if(msgType == MsgType.All)
			type = "0";
		else if(msgType == MsgType.New)
			type = "1";
		else 
			throw new ComClientException("Not supported message type");
		StringBuilder sb = new StringBuilder().append(openNum).append("\n").append(openPin).append("\n").append(type);
		client.Send(ComClient.cmdMsgGetIds, sb.toString());

		ArrayList<MessageId> out = new ArrayList<MessageId>(); 
		for(String s : client.getReplyString().split("\n")) {
			if(!s.isEmpty()) {
				try {
					out.add(new MessageId(s));
				}
				catch(Exception e) {
					System.out.println(e);
				}
			}
		}
		return out;
	}
	
	synchronized public String sendMsg(final String toNum, final String msg) throws ComClientException {
		
        if(traceOn)
        	System.out.println("TRACE : sendMsg() ");

		if(client == null)
			throw new ComClientException("Not connected");
		if(openNum == null)
			throw new ComClientException("Number not open.");
		if(toNum == null || msg == null)
			throw new ComClientException("toNum == null || msg == null");
		if(toNum.length() == 0)
			throw new ComClientException("Assert toNum.len = 0");
		
		StringBuilder sb = new StringBuilder().append(openNum).append("\n").append(openPin).append("\n")
				.append(toNum).append("\n").append(msg);
		client.Send(ComClient.cmdMsgSend, sb.toString());

		return client.getReplyString();
	}

	synchronized public String sendMsgToken(final String fromNum, final String toNum, final String token, final String msg) throws ComClientException {
		
        if(traceOn)
        	System.out.println("TRACE : sendMsgToken() ");

		if(client == null)
			throw new ComClientException("Not connected");
		
		StringBuilder sb = new StringBuilder().append(fromNum).append("\n").append(token).append("\n").append(toNum).append("\n").append(msg);
		client.Send(ComClient.cmdMsgSend, sb.toString());

		return client.getReplyString();
	}

	synchronized public void open(final String num, final String pin) throws ComClientException {
		
        if(traceOn)
        	System.out.println("TRACE : open() ");

		if(client == null)
			throw new ComClientException("Not connected");
		if(num == null || pin == null)
			throw new ComClientException("Assert num == null || pin == null");
		if(num.length() == 0 || pin.length() == 0)
			throw new ComClientException("Assert num.len == 0 || pin.len == 0");
		
		StringBuilder sb = new StringBuilder().append(num).append("\n").append(pin).append("\n");
		client.Send(ComClient.cmdOpen, sb.toString());
		
		openNum = num;
		openPin = pin;
	}

	synchronized public void changePin(final String num, final String pin, final String newPin) throws ComClientException {
		
        if(traceOn)
        	System.out.println("TRACE : changePin() ");

		if(client == null)
			throw new ComClientException("Not connected");
		if(num == null || pin == null || newPin == null)
			throw new ComClientException("Assert num == null || pin == null || newPin == null");
		if(num.length() == 0 || pin.length() == 0 || newPin.length() == 0)
			throw new ComClientException("Assert num.len == 0 || pin.len == 0 || newPin.len == 0");
		
		StringBuilder sb = new StringBuilder().append(num).append("\n").append(pin).append("\n").append(newPin).append("\n");
		client.Send(ComClient.cmdChangePin, sb.toString());
		
		openNum = num;
		openPin = pin;
	}
	
	synchronized public Message readMsg(final String msgId) throws ComClientException {
		
        if(traceOn)
        	System.out.println("TRACE : readMsg() ");

		if(client == null)
			throw new ComClientException("Not connected");
		if(openNum == null)
			throw new ComClientException("Number not open.");
		if(msgId == null)
			throw new ComClientException("msg == null");
		if(msgId.length() == 0)
			throw new ComClientException("Assert msgId.len = 0");
		
		StringBuilder sb = new StringBuilder().append(openNum).append("\n").append(openPin).append("\n").append(msgId);
		client.Send(ComClient.cmdMsgRead, sb.toString());

		Message out = new Message(client.getReplyString());
		return out;
	}
	
	synchronized public void deleteMsg(final String msgId) throws ComClientException {
		
        if(traceOn)
        	System.out.println("TRACE : deleteMsg() ");

		if(client == null)
			throw new ComClientException("Not connected");
		if(openNum == null)
			throw new ComClientException("Number not open.");
		if(msgId == null)
			throw new ComClientException("msg == null");
		if(msgId.length() == 0)
			throw new ComClientException("Assert msgId.len = 0");
		
		StringBuilder sb = new StringBuilder().append(openNum).append("\n").append(openPin).append("\n").append(msgId);
		client.Send(ComClient.cmdMsgDelete, sb.toString());
	}
	
	synchronized public MessageCount getMsgCount() throws ComClientException {
		
        if(traceOn)
        	System.out.println("TRACE : getMsgCount() ");

		if(client == null)
			throw new ComClientException("Not connected");
		if(openNum == null)
			throw new ComClientException("Number not open.");		
		
		StringBuilder sb = new StringBuilder().append(openNum).append("\n").append(openPin).append("\n");
		client.Send(ComClient.cmdMsgGetCount, sb.toString());

		MessageCount out = new MessageCount();
		// Should I move following implementation into constructor ?
		final String reply = client.getReplyString();
		String values[] = reply.split(" ");
		if(values.length != 2)
			throw new ComClientException("Unexpected reply. " + reply);
		out.totalMsgs = Integer.parseInt(values[0]);
		out.newMsgs = Integer.parseInt(values[1]);
		return out;
	}
	
	synchronized public int getTimeoutSec() {

        if(traceOn)
        	System.out.println("TRACE : getTimeoutSec() ");
		if(client != null)			
			return client.getTimeoutSec();
		else
			return preSetTimeOutSec;
	}
	
	synchronized public void setTimeoutSec(int sec) throws ComClientException {
		
        if(traceOn)
        	System.out.println("TRACE : setTimeoutSec() ");

        if(client != null)
        	client.setTimeoutSec(sec);
        else
        	preSetTimeOutSec = sec;
	}
	
	synchronized public String getDiag() throws ComClientException {
		
        if(traceOn)
        	System.out.println("TRACE : getGetDiag() ");

		if(client == null)
			throw new ComClientException("Not connected");
		client.Send(ComClient.cmdGetDiag, "");

		final String reply = client.getReplyString();
		return reply;
	}
	
	synchronized public String getNumToken() throws ComClientException {

        if(traceOn)
        	System.out.println("TRACE : getNumToken() ");

		if(client == null)
			throw new ComClientException("Not connected");
		if(openNum == null)
			throw new ComClientException("Number not open.");		
		
		StringBuilder sb = new StringBuilder().append(openNum).append("\n").append(openPin).append("\n");
		client.Send(ComClient.cmdTokenGet, sb.toString());

		final String reply = client.getReplyString();
		return reply;
	}
	
	synchronized public String newNumToken() throws ComClientException {

        if(traceOn)
        	System.out.println("TRACE : newNumToken() ");
		
		if(client == null)
			throw new ComClientException("Not connected");
		if(openNum == null)
			throw new ComClientException("Number not open.");		
		
		StringBuilder sb = new StringBuilder().append(openNum).append("\n").append(openPin).append("\n");
		client.Send(ComClient.cmdTokenNew, sb.toString());

		final String reply = client.getReplyString();
		return reply;
	}
	
	synchronized public void deleteNumToken() throws ComClientException {

        if(traceOn)
        	System.out.println("TRACE : deleteNumToken() ");
		
		if(client == null)
			throw new ComClientException("Not connected");
		if(openNum == null)
			throw new ComClientException("Number not open.");		
		
		StringBuilder sb = new StringBuilder().append(openNum).append("\n").append(openPin).append("\n");
		client.Send(ComClient.cmdTokenDelete, sb.toString());
	}
	
	synchronized public Boolean checkNumToken(final String num, final String token) throws ComClientException {

        if(traceOn)
        	System.out.println("TRACE : checkNumToken() ");
		
		if(client == null)
			throw new ComClientException("Not connected");
		
		StringBuilder sb = new StringBuilder().append(num).append("\n").append(token).append("\n");
		client.Send(ComClient.cmdTokenCheck, sb.toString());

		final String reply = client.getReplyString();
		return reply.equals("true");
	}
	
	public String getHost() throws ComClientException {
		
		if(client == null)
			throw new ComClientException("Client not connected");
		return client.getHost();
	}
	
	public int getPort() throws ComClientException {
		
		if(client == null)
			throw new ComClientException("Client not connected");
		return client.getPort();
	}
}
