package com.zs.client;

import java.util.ArrayList;

import com.zs.client.client.ZsClient;
import com.zs.client.client.ZsClient.Message;
import com.zs.client.client.ZsClient.MessageCount;
import com.zs.client.client.ZsClient.MessageId;
import com.zs.client.client.ZsClient.MsgType;

public class Test2 {

	private ZsClient client1 = null;
	private ZsClient client2 = null;
	private String num1 = null;
	private String num2 = null;
	private boolean noiseStop = false;
	
	private int timeOutSec = 10;
	private int port = 0;
	private String host;
	
	public void Start(final String host, int port, final String fromNum, final String toNum, int iter) throws Exception {

		this.host = host;
		this.port = port;
		
		client1 = new ZsClient();
		client2 = new ZsClient();

		//client1.setTrace(true);
		//client2.setTrace(true);
		if(iter < 1)
			iter = 1;
		
		System.out.println("Test 2 params. " + host + ":" + port + ", fromNum=" + fromNum + ", toNum=" + toNum + ", iters=" + iter);
		if(fromNum.equals(toNum))
			throw new Exception("Not allowed to have same number.");
		num1 = fromNum;
		num2 = toNum;
		
		client1.connect(host, port, timeOutSec);
		client2.connect(host, port, timeOutSec);
		
		//client1.setTrace(true);
		//client2.setTrace(true);
		
		for(int i = 0; i < iter; ++i) {			
			oneWriterOneSender_OneWay();
			echo();
			System.out.println("Done. iter " + i + "/" + iter);
		}
	}
	
	private void deleteNum(final ZsClient client, final String num, final String pin) throws Exception {

		try {
			client.deleteNumber(num, num);
		}
		catch(Exception e) {
			if(e.getMessage().indexOf("Number does not exist") == -1)
				throw new Exception(e.getMessage());
		}		
	}
	
	private String ReadMsg(final ZsClient client, final String checkMsgId, final String checkFrom, final String checkMsg) throws Exception {
		
		MessageCount count = client.getMsgCount();
		if(count.newMsgs != 1)
			throw new Exception("newCount = " + count.newMsgs + ", expected 1");
		final ArrayList<MessageId> msgs = client.getMsgIds(MsgType.New);
		if(msgs.size() != 1)
			throw new Exception("msgs.size() = " + msgs.size() + ", expected 1");
		
		MessageId m = msgs.get(0);
		if(!m.id.equals(checkMsgId))
			throw new Exception("invalid msgId = " + m.id + ", expected " + checkMsgId);
		
		if(!m.fromNum.equals(checkFrom))
			throw new Exception("invalid fromNum = " + m.fromNum + ", expected " + checkFrom);

		final Message msg = client.readMsg(m.id);
		if(!msg.fromNum.equals(checkFrom))
			throw new Exception("invalid 2 msg.fromNum = " + msg.fromNum + ", expected " + checkFrom);
		if(!msg.status.equals("New"))
			throw new Exception("invalid msg.status = " + msg.status + ", expected " + "New");
		if(!msg.text.equals(checkMsg))
			throw new Exception("invalid msg.text = " + msg.text + ", expected " + checkMsg);
		
		// Clean device.
		{
			final ArrayList<MessageId> all = client.getMsgIds(MsgType.All);
			if(all.size() >= 512) {
				// When half of the storage is full, start to clean up.
				for(MessageId one : all)
					client.deleteMsg(one.id);
				int n = client.getMsgIds(MsgType.All).size();
				if(n != 0)
					throw new Exception("Failed to delete all messages. size = " + n);
			}
		}
		return msg.text;
	}

	private class ReaderNoise extends Thread {
		public void run() {
			try {
				ZsClient client = new ZsClient();
				client.connect(host, port, timeOutSec);
				client.open(num2, num2);
				while(noiseStop != true) {
					client.getMsgIds(MsgType.All);
					Thread.sleep(100);
				}
			}
			catch(Exception e) {
				System.out.println("Noise error : " + e.getMessage());
			}
		}
	}
	
	// Sender sends to reader messages of different length.
	// Reader reads and check content.
	private void oneWriterOneSender_OneWay() throws Exception {
		
		System.out.println("--- oneWriterOneSender_OneWay");
		
		deleteNum(client1, num1, num1);
		deleteNum(client2, num2, num2);
		
		num1 = client1.addNumber(num1, num1);
		num2 = client2.addNumber(num2, num2);

		client1.open(num1, num1);
		client2.open(num2, num2);
		
		int totalNoise = 10;
		for(int i = 0; i < totalNoise; ++i) {
			ReaderNoise t = new ReaderNoise();
			t.start();
		}
		System.out.println("Total noise = " + totalNoise);
		
		ArrayList<String> lines = new ArrayList<String>();
		String one = "";
		while(true) {
			one += java.util.UUID.randomUUID().toString();
			if(one.length() > 4 * 1024)
				break;
			lines.add(one);
		}
		
		long startTime = System.currentTimeMillis();
		int total = 0, totalLines = lines.size();
		int max = 1000;		
		int n = totalLines * max; 
		for(String line : lines) {		
			for(int i = 0; i < max; ++i) {
				if(total % 10000 == 0)
					System.out.println(total + "/" + n);
				final String id = client1.sendMsg(num2, line);
				ReadMsg(client2, id, num1, line);
				++total;
			}
		}
		long endTime = System.currentTimeMillis();
		float seconds = (endTime - startTime) / 1000;
		float msgPerSec = (float) (total / seconds);
		System.out.println("Messages " + total + ", " + seconds + " sec, " + msgPerSec + " msg/sec");
		noiseStop = true;
	}
	
	private void echo() throws Exception {
		
		int max = 1000000;
		System.out.println("--- echo " + max + " messages.");
		
		deleteNum(client1, num1, num1);
		deleteNum(client2, num2, num2);
		
		num1 = client1.addNumber(num1, num1);
		num2 = client2.addNumber(num2, num2);

		client1.open(num1, num1);
		client2.open(num2, num2);
		
		long startTime = System.currentTimeMillis();
		int count = 123;
		
		ZsClient curWriter = client1;
		ZsClient curReader = client2;
		String curWriterNum = num1;
		String curReaderNum = num2;
		
		ZsClient tmp = null;
		String s = null;
		for(int i = 0; i < max; ++i) {
			if(i % 10000 == 0)
				System.out.println(i + "/" + max);
			
			String msg = "" + count;
			String id = curWriter.sendMsg(curReaderNum, msg);
			String read = ReadMsg(curReader, id, curWriterNum, msg);
			if(!msg.equals(read))
				throw new Exception("Failed. expected " + msg + ", got " + read);
			++count;

			// swap reader and writer.
			tmp = curReader;
			curReader = curWriter;
			curWriter = tmp;
			
			s = curReaderNum;
			curReaderNum = curWriterNum;
			curWriterNum = s;
		}
		
		long endTime = System.currentTimeMillis();
		float seconds = (endTime - startTime) / 1000;
		float msgPerSec = (float) (max / seconds);
		System.out.println("Messages " + max + ", " + seconds + " sec, " + msgPerSec + " msg/sec");
	}
}
