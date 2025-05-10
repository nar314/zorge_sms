package com.zs.client;

import java.text.SimpleDateFormat;
import java.util.ArrayList;

import com.zs.client.client.ZsClient;
import com.zs.client.client.ZsClient.Message;
import com.zs.client.client.ZsClient.MessageCount;
import com.zs.client.client.ZsClient.MessageId;
import com.zs.client.client.ZsClient.MsgType;

public class TerminalExecutor {

	static void help(ZsClient client) {
		String out = 	"help - get this help.\n" +
						"trace <on/off>\n" +
						"set timeout <seconds>\n" +
						"connect <server> <port>			: localhost 7530\n" + 
						"diag                               : get diagnostics\n" +
						"num add <num> <pin>				: num add 101 9999\n" +
						"num del <num> <pin>				: num del 101 9999\n" +
						"num chagePin <num> <pin> <newPin> 	: num changePin 101 9999 8080\n" +
						"\n" +
						"open <num> <pin>					: open 101 9999\n" +
						"close\n" +		
						"\nYou have to call open for following calls:\n" +
						"msg check\n" +
						"msg getIds <new/all>				: msg getIds new\n" +
						"msg send <toNum> <Message>			: msg send 202 This is the message\n" +
						"msg read <msgId>					: msg read 1023-1af2-4336\n" +
						"msg del <msgId>					: msg del 1023-1af2-4336\n" +
						"token get\n" +
						"token new\n" +
						"token delete\n" +
						"token check <num> <token>          : token check 101 779ACF04655C45528597F79B15A39F50\n" +
						"\n\n" +
						"test <func> <server> <port> <fromNum> : test func localhost 7530 101\n" +
						"test 2 <server> <port> <fromNum> <toNum> : test 2 localhost 7530 101 102 1";
		
		System.out.println(out);
		System.out.print("\n\n");
	}
	
	static void connect(final String[] tokens, ZsClient client) throws Exception {
	 
		// connect localhost 7530
		if(tokens.length != 3)
			throw new Exception("Invalid argument count. Expected : connect localhost 7777");		
		client.connect(tokens[1], Integer.parseInt(tokens[2]));
		System.out.println("Connected.");
		System.out.println("Timeout " + client.getTimeoutSec() + " sec.");
	}

	static void number(final String[] tokens, ZsClient client) throws Exception {
		 
		int size = tokens.length;
		if(size < 2)
			throw new Exception("Invalid argument count.");
		final String cmd = tokens[1];
		if(cmd.equals("add")) {
			// num add 01-02-03 1122
			if(size != 4)
				throw new Exception("Invalid argument count for 'add'. Expected : num add 01-02-03 1111");
			final String newNum = client.addNumber(tokens[2], tokens[3]);
			System.out.println("New number added : " + newNum);
		}
		else if(cmd.equals("del")) {
			// num del 01-02-03 1122
			if(size != 4)
				throw new Exception("Invalid argument count for 'del'. Expected : num del 01-02-03 1111");
			client.deleteNumber(tokens[2], tokens[3]);
			System.out.println("Number deleted.");
		}
		else if(cmd.equals("changePin")) {
			// num changePin 101 9999 9090
			if(size != 5)
				throw new Exception("Invalid argument count for 'changePin'. Expected : num changePin 01-02-03 1111 1213");			
			client.changePin(tokens[2], tokens[3], tokens[4]);
			System.out.println("Pin changed.");
		}
		else
			throw new Exception("Not supported command :" + cmd);
	}

	static void message(final String cmdLine, final String[] tokens, ZsClient client) throws Exception {
		 
		int size = tokens.length;
		if(size < 2)
			throw new Exception("Invalid argument count.");
		final String cmd = tokens[1];
		
		if(cmd.equals("getIds")) {
			// msg getIds new
			// msg getIds all
			if(size != 3)
				throw new Exception("Invalid argument count for 'getIds'. Expected : msg getIds new");
			
			final String type = tokens[2];
			MsgType msgType = MsgType.All;
			if(type.equals("all"))
				msgType = MsgType.All;
			else if(type.equals("new"))
				msgType = MsgType.New;
			else
				throw new Exception("Invalid argument value for 'getIds'. Expected : msg getIds new");
			
			SimpleDateFormat df = new SimpleDateFormat("dd MMM yyyy HH:mm:ss");
			ArrayList<MessageId> ids = client.getMsgIds(msgType);
			System.out.println("Total ids : " + ids.size());
			for(MessageId id : ids) {
				System.out.println(id.status + " {" + df.format(id.dateLocal) + "} [" + id.fromNum + "] " + id.id);
			}
		}
		else if(cmd.equals("send")) {
			// msg send 505 This the message to be send.
			if(size < 3)
				throw new Exception("Invalid argument count for 'send'. Expected : msg send 505 This the message to be send.");
			
			int skip = 3 + 1 + 4 + 1 + tokens[2].length() + 1;
			String msg = cmdLine.substring(skip);

			final String msgId = client.sendMsg(tokens[2], msg);
			System.out.println("New message id " + msgId);
		}
		else if(cmd.equals("read")) {
			// msg read 1023-1af2-4336
			if(size < 3)
				throw new Exception("Invalid argument count for 'read'. Expected : msg read 1023-1af2-4336");
			
			final Message m = client.readMsg(tokens[2]);
			SimpleDateFormat df = new SimpleDateFormat("dd MMM yyyy HH:mm:ss");

			System.out.println("Status : " + m.status);
			System.out.println("Sent : " + df.format(m.dateLocal));
			System.out.println("From num : " + m.fromNum);
			System.out.println("Text : " + m.text);
		}	
		else if(cmd.equals("del")) {
			// msg del 1023-1af2-4336
			if(size < 3)
				throw new Exception("Invalid argument count for 'del'. Expected : msg del 1023-1af2-4336");
			
			client.deleteMsg(tokens[2]);
			System.out.println("Message deleted.");
		}
		else if(cmd.equals("check")) {
			// msg check
			if(size != 2)
				throw new Exception("Invalid argument count for 'check'. Expected : msg check");
			
			MessageCount count = client.getMsgCount();
			System.out.println("Total messages : " + count.totalMsgs);
			System.out.println("New messages : " + count.newMsgs);
		}		
		else
			throw new Exception("Not supported command :" + cmd);
	}
	
	static void open(final String[] tokens, ZsClient client) throws Exception {
		 
		int size = tokens.length;
		if(size < 1)
			throw new Exception("Invalid argument count.");

		final String cmd = tokens[0];
		if(cmd.equals("open")) {
			// open 101 9999
			if(size != 3)
				throw new Exception("Invalid argument count for 'open'. Expected : open 101 9999");
			
			client.open(tokens[1], tokens[2]);
		}
		else
			throw new Exception("Not supported command :" + cmd);		
	}

	static void trace(final String[] tokens, ZsClient client) throws Exception {
		 
		int size = tokens.length;
		if(size < 1)
			throw new Exception("Invalid argument count.");

		final String cmd = tokens[1];
		if(cmd.equals("on")) {
			client.setTrace(true);
			System.out.println("trace set ON.");
		}
		else if(cmd.equals("off")) {
			client.setTrace(false);
			System.out.println("trace set OFF.");
		}
		else
			throw new Exception("Not supported command :" + cmd);		
	}

	static void diag(ZsClient client) throws Exception {
		 
		System.out.println(client.getDiag());
	}

	static void set(final String[] tokens, ZsClient client) throws Exception {
		 
		int size = tokens.length;
		if(size < 2)
			throw new Exception("Invalid argument count.");
		final String cmd = tokens[1];
		if(cmd.equals("timeout")) {
			// set timeout 1
			if(size != 3)
				throw new Exception("Invalid argument count for 'set timeout'. Expected : set timeout 1");			
			client.setTimeoutSec(Integer.parseInt(tokens[2]));
			System.out.println("Current timeout : " + client.getTimeoutSec() + " sec.");
		}		
		else
			throw new Exception("Not supported command :" + cmd);		
	}
	
	static void test(final String[] tokens, ZsClient client) throws Exception {

		boolean trace = client != null ? client.isTraceOn() : false;
		int size = tokens.length;
		if(size == 1) {
			Test1 test1 = new Test1();
			test1.Start("localhost", 7530, "101", trace);
			return;
		}
		
		if(size < 5)
			throw new Exception("Invalid argument count.");
		final String cmd = tokens[1];
		if(cmd.equals("func")) {
			// test func localhost 7530 101";
			Test1 test1 = new Test1();
			test1.Start(tokens[2], Integer.parseInt(tokens[3]), tokens[4], trace);			
		}
		else if(cmd.equals("2")) {
			// test 2 localhost 7530 101 102 1
			if(size < 6)
				throw new Exception("Invalid paramater count for 'test 2'. Expected : test 2 localhost 7530 101 102 1");
			int iter = 1;
			if(size == 7)
				iter = Integer.valueOf(tokens[6]);
			
			Test2 test2 = new Test2();
			test2.Start(tokens[2], Integer.parseInt(tokens[3]), tokens[4], tokens[5], iter);
		}
		else
			throw new Exception("Not supported command :" + cmd);		
	}
	
	static void token(final String cmdLine, final String[] tokens, ZsClient client) throws Exception {
		 
		int size = tokens.length;
		if(size < 2)
			throw new Exception("Invalid argument count.");
		
		final String cmd = tokens[1];
		
		if(cmd.equals("get")) {
			// token get
			final String token = client.getNumToken();
			System.out.println("Current token : " + token);
		}
		else if(cmd.equals("new")) {
			// token new
			final String token = client.newNumToken();
			System.out.println("New token : " + token);
		}
		else if(cmd.equals("delete")) {
			// token delete
			client.deleteNumToken();
			System.out.println("Token deleted");
		}
		else if(cmd.equals("check")) {
			if(size != 4)
				throw new Exception("Invalid argument count.");
			
			// token check 101 779ACF04655C45528597F79B15A39F50
			Boolean bValid = client.checkNumToken(tokens[2], tokens[3]);
			if(bValid)
				System.out.println("Token is valid");
			else
				System.out.println("Token is not valid");
		}				
		else
			throw new Exception("Not supported command :" + cmd);
	}

	static void executeCommand(final String cmd, ZsClient client) throws Exception {
		
		if(cmd == null || client == null)
			throw new Exception("Assert cmd == null || client == null");
		
		if(cmd.trim().length() == 0)
			return;
		
		String tokens[] = cmd.split(" ");
		final String command = tokens[0];
		if(command.equals("help"))
			help(client);
		else if(command.equals("connect"))
			connect(tokens, client);
		else if(command.equals("diag"))
			diag(client);
		else if(command.equals("open"))
			open(tokens, client);
		else if(command.equals("close"))
			client.close();
		else if(command.equals("num"))
			number(tokens, client);
		else if(command.equals("msg"))
			message(cmd, tokens, client);
		else if(command.equals("token"))
			token(cmd, tokens, client);		
		else if(command.equals("trace"))
			trace(tokens, client);
		else if(command.equals("set"))
			set(tokens, client);
		else if(command.equals("test"))
			test(tokens, client);
		else
			System.out.println("Not supported command :"+ cmd);
	}
}
