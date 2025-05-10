package com.zs.client;

import java.util.ArrayList;
import java.util.HashMap;

import com.zs.client.client.ComClientException;
import com.zs.client.client.ZsClient;
import com.zs.client.client.ZsClient.Message;
import com.zs.client.client.ZsClient.MessageCount;
import com.zs.client.client.ZsClient.MessageId;
import com.zs.client.client.ZsClient.MsgType;

public class Test1 {

	private ZsClient client = null;
	private String areaCode = "" + System.currentTimeMillis();
	
	private int totalNumbers = 1000;
	private int totalMessages = 1024;
		
	public void Start(final String host, int port, final String fromNum, final boolean trace) throws Exception {
		
		client = new ZsClient();
		client.setTrace(trace);
		
		areaCode = fromNum;
		System.out.println("Numbers in use : " + fromNum + ", totalNumbers :" + totalNumbers);
		client.connect(host, port, 10); // 10 secs timeout

		testToken();
		testToken_Invalid();
		
		testChangePin();
		testChangePin_Invalid();
		
		testOpenNum_Invalid();
		testAddNumber_DeleteNumber();
		
		testAddNumber_Invalid();
		testDeleteNumber_Invalid();
		
		testSendMsg();
		testSendMsg_Invalid();
		
		testDeleteMsg();
		testDeleteMsg_Invalid();
		
		testReadMsg();
		testReadMsg_Invalid();
		
		testReadMsg2();
		testGetMsgCount();

		System.out.println("Done.");
	}

	private void deleteNum(final String num, final String pin) throws Exception {

		try {
			client.deleteNumber(num, num);
		}
		catch(Exception e) {
			if(e.getMessage().indexOf("Invalid pin") != -1) {
				client.deleteNumber(num, num + num);
			}
			else if(e.getMessage().indexOf("Number does not exist") == -1)
				throw new Exception(e.getMessage());
		}		
	}

	//--------------------------------------------------------------------------------------------
	//
	//--------------------------------------------------------------------------------------------	
	private void testDeleteNumber_Invalid() throws Exception {
		
		System.out.print("-- testDeleteNumber_Invalid() : ");
		
		String num = areaCode;
		deleteNum(num, num);
		client.addNumber(num, num);
	
		// number not found
		try {
			client.deleteNumber("991192929191", num);
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Number does not exist") == -1)				
				throw e;
		}
		// number is empty
		try {
			client.deleteNumber("", num);
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Assert") == -1)				
				throw e;
		}
		// number is too long
		try {
			String tooLong = "12345678901234567"; // 16
			client.deleteNumber(tooLong, num);
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Number is too long") == -1)				
				throw e;
		}
		// number is too short
		try {
			String tooLong = "11"; // 3
			client.deleteNumber(tooLong, num);
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Number is too short") == -1)				
				throw e;
		}		
		// pin is invalid
		try {
			client.deleteNumber(num, "invalid pin");
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Invalid pin") == -1)				
				throw e;
		}
		// pin is empty
		try {
			client.deleteNumber(num, "");
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Assert") == -1)				
				throw e;
		}
		// pin is too long
		try {
			String tooLong = ""; // 64
			for(int i = 0; i < 65; ++i)
				tooLong += "1";				
			client.deleteNumber(num, tooLong);
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Pin is too long") == -1)				
				throw e;
		}
		// pin is too short
		try {
			client.deleteNumber(num, "11"); // 3
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Pin is too short") == -1)				
				throw e;
		}
		System.out.println("Done.");
	}
	
	//--------------------------------------------------------------------------------------------
	//
	//--------------------------------------------------------------------------------------------	
	private void testAddNumber_Invalid() throws Exception {
		
		System.out.print("-- AddNumber_Invalid() : ");
		
		String num = areaCode;
		deleteNum(num, num);
		client.addNumber(num, num);

		// number already exist
		try {
			client.addNumber(num, num);
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Number already exist") == -1)				
				throw e;
		}
		// number is empty
		try {
			client.addNumber("", num);
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Assert") == -1)				
				throw e;
		}		
		// number is too long
		try {
			String tooLong = ""; // 256
			for(int i = 0; i < 257; ++i)
				tooLong += "1";				
			client.addNumber(tooLong, num);
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Number is too long") == -1)				
				throw e;
		}
		// number is too short
		try {
			client.addNumber("11", num); // 3
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Number is too short") == -1)				
				throw e;
		}		
		// pin is empty
		try {
			client.addNumber(num, "");
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Assert") == -1)				
				throw e;
		}				
		// pin is too long
		try {
			String tooLong = ""; // 64
			for(int i = 0; i < 65; ++i)
				tooLong += "1";	
			client.addNumber(num, tooLong);
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Pin is too long") == -1)				
				throw e;
		}
		System.out.println("Done.");
	}
	
	//--------------------------------------------------------------------------------------------
	//
	//--------------------------------------------------------------------------------------------
	private void testAddNumber_DeleteNumber() throws Exception {
		
		ArrayList<String> nums = new ArrayList<String>();

		System.out.print("-- AddNumber : ");
		String num;
		for(int i = 0; i < totalNumbers; ++i) {
			num = areaCode + i;
			deleteNum(num, num);
			nums.add(client.addNumber(num, num));
		}

		for(String n : nums)
			client.open(n, n);
		
		for(String n : nums)
			client.deleteNumber(n, n);

		for(String n : nums) {
			try {
				client.open(n, n);
				throw new Exception("Assert !");
			}
			catch(Exception e) {
				final String s = e.getMessage();
				if(s.indexOf("Number does not exist") == -1)
					throw new Exception(s);
			}
		}
		System.out.println("Done. totalNumbers = " + totalNumbers);
	}
	
	//--------------------------------------------------------------------------------------------
	//
	//--------------------------------------------------------------------------------------------
	private void testSendMsg() throws Exception {
		
		System.out.print("-- testSendMsg : ");
		
		String num1 = areaCode + "1";
		String num2 = areaCode + "2";
		
		deleteNum(num1, num1);
		deleteNum(num2, num2);

		num1 = client.addNumber(num1, num1);
		num2 = client.addNumber(num2, num2);
		
		client.open(num1, num1);
		
		HashMap<String, String> msgIds = new HashMap<String, String>();
		final String msg = "This is message ";
		for(int i = 0; i < totalMessages; ++i) {
			final String id = client.sendMsg(num2, msg + i);
			msgIds.put(id, msg);
		}
		
		client.open(num2, num2);
		// Check new messages
		ArrayList<MessageId> loadedIds = client.getMsgIds(MsgType.New);
		if(loadedIds.size() != totalMessages)
			throw new Exception("Failed. loadedIds.size() = " + loadedIds.size() + ", totalMessages = " + totalMessages);
		
		for(MessageId m : loadedIds) {
			if(msgIds.get(m.id) == null)
				throw new Exception("not found id. m.id = " + m.id);
		}
		
		// Check all messages
		loadedIds = client.getMsgIds(MsgType.All);
		if(loadedIds.size() != totalMessages)
			throw new Exception("Failed. loadedIds.size() = " + loadedIds.size() + ", totalMessages = " + totalMessages);
		
		for(MessageId m : loadedIds) {
			if(msgIds.get(m.id) == null)
				throw new Exception("not found id. m.id = " + m.id);
		}
		System.out.println("Done. totalMessages = " + totalMessages);
	}
	
	//--------------------------------------------------------------------------------------------
	//
	//--------------------------------------------------------------------------------------------
	private void testSendMsg_Invalid() throws Exception {

		System.out.print("-- testSendMsg_Invalid() : ");
		
		String num = areaCode;
		deleteNum(num, num);
		client.addNumber(num, num);
		client.open(num, num);

		// number does not exist		
		try {
			client.sendMsg("9911991199", "");
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Number 'to' does not exist") == -1)				
				throw e;
		}
		// number is empty		
		try {
			client.sendMsg("", "");
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Assert") == -1)				
				throw e;
		}
		// number is too long
		try {
			String tooLong = "12345678901234567"; // 16			
			client.sendMsg(tooLong, "");
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Number is too long") == -1)				
				throw e;
		}
		// message is too long
		try {
			String tooLong = ""; // 4 KB
			for(int i = 0; i < 4 * 1024 + 1; ++i)
				tooLong += "1";	
			client.sendMsg(num, tooLong);
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Message is too long") == -1)				
				throw e;
		}
		System.out.println("Done.");
	}

	//--------------------------------------------------------------------------------------------
	//
	//--------------------------------------------------------------------------------------------	
	private void testDeleteMsg_Invalid() throws Exception {

		System.out.print("-- testDeleteMsg_Invalid : ");
		
		String num = areaCode;
		deleteNum(num, num);
		client.addNumber(num, num);	
		client.open(num, num);

		// id does not exist
		try {
			client.deleteMsg("id does not exist");
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Message not found") == -1)				
				throw e;
		}				
		// id is empty
		try {
			client.deleteMsg("");
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Assert") == -1)				
				throw e;
		}						
		// id is too long. No such thing for now.
		System.out.println("Done.");
	}
	
	//--------------------------------------------------------------------------------------------
	//
	//--------------------------------------------------------------------------------------------
	private void testDeleteMsg() throws Exception {
		
		System.out.print("-- testDeleteMsg : ");
		
		String num1 = areaCode + "1";
		String num2 = areaCode + "2";
		
		deleteNum(num1, num1);
		deleteNum(num2, num2);

		num1 = client.addNumber(num1, num1);
		num2 = client.addNumber(num2, num2);
		
		client.open(num1, num1);
		
		HashMap<String, String> msgIds = new HashMap<String, String>();
		final String msg = "This is message ";
		for(int i = 0; i < totalMessages; ++i) {
			final String id = client.sendMsg(num2, msg + i);
			msgIds.put(id, msg);
		}
		
		client.open(num2, num2);
		ArrayList<MessageId> loadedIds = client.getMsgIds(MsgType.All);
		if(loadedIds.size() != totalMessages)
			throw new Exception("Failed. loadedIds.size() = " + loadedIds.size() + ", totalMessages = " + totalMessages);

		for(MessageId m : loadedIds)
			client.deleteMsg(m.id);
		
		loadedIds = client.getMsgIds(MsgType.All);
		if(loadedIds.size() != 0)
			throw new Exception("Failed. 1. loadedIds.size() = " + loadedIds.size());
		System.out.println("Done. totalMessages = " + totalMessages);
	}

	//--------------------------------------------------------------------------------------------
	//
	//--------------------------------------------------------------------------------------------	
	private void testReadMsg_Invalid() throws Exception {

		System.out.print("-- testReadMsg_Invalid : ");
		
		String num = areaCode;
		deleteNum(num, num);
		client.addNumber(num, num);	
		client.open(num, num);

		// no messages
		try {
			client.readMsg("id does not exist");
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("No messages") == -1)				
				throw e;
		}
		// id does not exist
		try {
			client.sendMsg(num, "send one messqage");			
			client.readMsg("id does not exist");
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Message not found") == -1)				
				throw e;
		}
		// id is empty
		try {
			client.readMsg("");
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Assert") == -1)				
				throw e;
		}
		// id is too long. No such thing.	
		System.out.println("Done.");
	}

	//--------------------------------------------------------------------------------------------
	//
	//--------------------------------------------------------------------------------------------
	private void testReadMsg() throws Exception {
		
		System.out.print("-- testReadMsg : ");
		
		String num1 = areaCode + "1";
		String num2 = areaCode + "2";
		
		deleteNum(num1, num1);
		deleteNum(num2, num2);

		num1 = client.addNumber(num1, num1);
		num2 = client.addNumber(num2, num2);
		
		client.open(num1, num1);
		
		HashMap<String, String> msgIds = new HashMap<String, String>();
		final String msg = "This is message ";
		for(int i = 0; i < totalMessages; ++i) {
			final String txt = msg + i;
			final String id = client.sendMsg(num2, txt);
			msgIds.put(id, txt);
		}
		
		client.open(num2, num2);
		ArrayList<MessageId> loadedIds = client.getMsgIds(MsgType.All);
		if(loadedIds.size() != totalMessages)
			throw new Exception("Failed. loadedIds.size() = " + loadedIds.size() + ", totalMessages = " + totalMessages);

		for(MessageId m : loadedIds) {
			String txtSent = msgIds.get(m.id);
			if(txtSent == null)
				throw new Exception("Failed. msgId not found = " + m.id);
			
			Message curMsg = client.readMsg(m.id);
			if(!curMsg.fromNum.equals(num1))
				throw new Exception("Failed. fromNum = " + curMsg.fromNum);
			
			if(!curMsg.text.equals(txtSent))
				throw new Exception("Failed. txtSent = " + txtSent + ", curMsg.text = " + curMsg.text);
		}
		
		System.out.println("Done. totalMessages = " + totalMessages);
	}
	
	//--------------------------------------------------------------------------------------------
	//
	//--------------------------------------------------------------------------------------------	
	private void testReadMsg2() throws Exception {
		
		System.out.print("-- testReadMsg2 : ");
		
		String num1 = areaCode + "3";
		String num2 = areaCode + "4";
		
		deleteNum(num1, num1);
		deleteNum(num2, num2);

		num1 = client.addNumber(num1, num1);
		num2 = client.addNumber(num2, num2);
		
		client.open(num1, num1);
		
		ArrayList<String> messages = new ArrayList<String>();
		messages.add(new String(""));
		messages.add(new String("\n"));
		messages.add(new String("\n\n\n"));
		messages.add(new String(" \n \n \n"));
		messages.add(new String(" \n \n \n "));
		messages.add(new String("то, что тебя не убьет, сделает тебя сильнее"));
		messages.add(new String("那些殺不死你的會讓你更強大"));
		
		HashMap<String, String> msgIds = new HashMap<String, String>();
		for(String txt : messages) {
			final String id = client.sendMsg(num2, txt);
			msgIds.put(id, txt);
		}
		
		client.open(num2, num2);
		ArrayList<MessageId> loadedIds = client.getMsgIds(MsgType.All);
		if(loadedIds.size() != messages.size())
			throw new Exception("Failed. loadedIds.size() = " + loadedIds.size() + ", messages.size() = " + messages.size());

		for(MessageId m : loadedIds) {
			String txtSent = msgIds.get(m.id);
			if(txtSent == null)
				throw new Exception("Failed. msgId not found = " + m.id);
			
			Message curMsg = client.readMsg(m.id);
			if(!curMsg.fromNum.equals(num1))
				throw new Exception("Failed. fromNum = " + curMsg.fromNum);
			
			if(!curMsg.text.equals(txtSent))
				throw new Exception("Failed. txtSent = " + txtSent + ", curMsg.text = " + curMsg.text);
		}
		System.out.println("Done. msgIds = " + msgIds.size());
	}
	
	//--------------------------------------------------------------------------------------------
	//
	//--------------------------------------------------------------------------------------------	
	private void testGetMsgCount() throws Exception {
		
		System.out.print("-- testGetMsgCount : ");
		
		String num1 = areaCode + "5";
		
		deleteNum(num1, num1);

		num1 = client.addNumber(num1, num1);
		
		client.open(num1, num1);
		int total = 1024;
		for(int i = 0; i < total; ++i)
			client.sendMsg(num1, "testGetMsgCount " + i);
		
		ArrayList<MessageId> loadedIds = client.getMsgIds(MsgType.All);
		if(loadedIds.size() != total)
			throw new Exception("Failed. loadedIds.size() = " + loadedIds.size() + ", total = " + total);

		int expected = total;
		for(MessageId m : loadedIds) {
			MessageCount n = client.getMsgCount();
			if(n.newMsgs != expected)
				throw new Exception("Failed. n = " + n.newMsgs + ", expected = " + expected);
			--expected;
			client.readMsg(m.id);
		}
		System.out.println("Done. total = " + total);
	}
	
	//--------------------------------------------------------------------------------------------
	//
	//--------------------------------------------------------------------------------------------	
	private void testOpenNum_Invalid() throws Exception {

		System.out.print("-- testOpenNum_Invalid : ");
		
		String num = areaCode;
		deleteNum(num, num);
		client.addNumber(num, num);
		
		// number does not exist
		try {
			client.open("998181826", num);
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Number does not exist") == -1)				
				throw e;
		}
		// number is empty
		try {
			client.open("", num);
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Assert") == -1)				
				throw e;
		}		
		// number is too long
		try {
			String tooLong = "12345678901234567"; // 16
			client.open(tooLong, num);
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Number is too long") == -1)				
				throw e;
		}		
		// number is too short
		try {
			client.open("11", num); // 3
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Number is too short") == -1)				
				throw e;
		}
		// invalid pin
		try {
			client.open(num, "invalid");
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Invalid pin") == -1)				
				throw e;
		}		
		// pin is empty
		try {
			client.open(num, "");
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Assert") == -1)
				throw e;
		}
		// pin is too long
		try {
			String tooLong = ""; // 64
			for(int i = 0; i < 65; ++i)
				tooLong += "1";	
			client.open(num, tooLong);
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Pin is too long") == -1)
				throw e;
		}		
		// pin is too short
		try {
			client.open(num, "11");
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Pin is too short") == -1)
				throw e;
		}
		System.out.println("Done.");
	}
	
	//--------------------------------------------------------------------------------------------
	//
	//--------------------------------------------------------------------------------------------	
	private void testChangePin() throws Exception {
		
		ArrayList<String> nums = new ArrayList<String>();

		System.out.print("-- ChangePin : ");
		String num;
		for(int i = 0; i < totalNumbers; ++i) {
			num = areaCode + i;
			deleteNum(num, num);
			nums.add(client.addNumber(num, num));
		}

		for(String n : nums)
			client.open(n, n);

		// Change pin
		for(String n : nums)
			client.changePin(n, n, n + n);

		for(String n : nums)
			client.open(n, n + n);
		
		// clean up
		for(String n : nums)
			client.deleteNumber(n, n + n);

		for(String n : nums) {
			try {
				client.open(n, n + n);
				throw new Exception("Assert !");
			}
			catch(Exception e) {
				final String s = e.getMessage();
				if(s.indexOf("Number does not exist") == -1)
					throw new Exception(s);
			}
		}
		System.out.println("Done. totalNumbers = " + totalNumbers);
	}

	//--------------------------------------------------------------------------------------------
	//
	//--------------------------------------------------------------------------------------------		
	private void testChangePin_Invalid() throws Exception {

		System.out.print("-- testChangePin_Invalid : ");
		String num = areaCode;
		deleteNum(num, num);
		client.addNumber(num, num);
		
		// number does not exist
		try {
			client.changePin("998181826", num, num);
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Number does not exist") == -1)				
				throw e;
		}
		// number is empty
		try {
			client.open("", num);
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Assert") == -1)				
				throw e;
		}		
		// number is too long
		try {
			String tooLong = "12345678901234567"; // 16
			client.open(tooLong, num);
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Number is too long") == -1)				
				throw e;
		}		
		// number is too short
		try {
			client.open("11", num); // 3
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Number is too short") == -1)				
				throw e;
		}
		// invalid pin
		try {
			client.open(num, "invalid");
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Invalid pin") == -1)				
				throw e;
		}		
		// pin is empty
		try {
			client.open(num, "");
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Assert") == -1)
				throw e;
		}
		// pin is too long
		try {
			String tooLong = ""; // 64
			for(int i = 0; i < 65; ++i)
				tooLong += "1";	
			client.open(num, tooLong);
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Pin is too long") == -1)
				throw e;
		}		
		// pin is too short
		try {
			client.open(num, "11");
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Pin is too short") == -1)
				throw e;
		}		
		System.out.println("Done.");
	}
	
	private void testToken() throws Exception {
		
		ArrayList<String> nums = new ArrayList<String>();

		System.out.print("-- testToken : ");
		String num = null;
		for(int i = 0; i < totalNumbers; ++i) {
			num = areaCode + i;
			deleteNum(num, num);
			nums.add(client.addNumber(num, num));
		}

		String token1, token2;
		for(String n : nums) {
			client.open(n, n);
			token1 = client.getNumToken();
			if(token1.isEmpty())
				throw new Exception("Empty token 1");
			
			token2 = client.newNumToken();
			if(token2.isEmpty())
				throw new Exception("Empty token 2");
			if(token1.equals(token2))
				throw new Exception("Equals tokens");
			
			client.deleteNumToken();
			token1 = client.getNumToken();
			if(!token1.isEmpty())
				throw new Exception("Token is not empty 1");
			
			// delete again
			client.deleteNumToken();
			token1 = client.getNumToken();
			if(!token1.isEmpty())
				throw new Exception("Token is not empty 2");			
		}

		System.out.println("Done. totalNumbers = " + totalNumbers);
	}
	
	private void testToken_Invalid() throws Exception {
		
		System.out.print("-- testToken_Invalid : ");
		String num = null;
		for(int i = 0; i < totalNumbers; ++i) {
			num = areaCode + i;
			deleteNum(num, num);
		}

		// Number does not exist
		try {
			client.getNumToken();
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Number does not exist") == -1)
				throw e;
		}				

		try {
			client.newNumToken();
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Number does not exist") == -1)
				throw e;
		}				

		try {
			client.deleteNumToken();
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Number does not exist") == -1)
				throw e;
		}				

		// Number not open
		num = areaCode + "0";
		client.addNumber(num, num);
		client.close();
		try {
			client.getNumToken();
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Number not open") == -1)
				throw e;
		}				

		try {
			client.newNumToken();
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Number not open") == -1)
				throw e;
		}				

		try {
			client.deleteNumToken();
		}
		catch(ComClientException e) {
			if(e.getMessage().indexOf("Number not open") == -1)
				throw e;
		}				
		
		System.out.println("Done.");
	}		
}
