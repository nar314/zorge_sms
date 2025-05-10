package com.zs.client.client;

import java.io.UnsupportedEncodingException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Random;

/**
 * Request builder. Request and respond have same structure.
 */
public class ComRequestBuilder {

	private short 	cmdId = 0;			// command id
	private byte 	clientId[] = null;	// client GUID
	private Coder 	coder = null;		// coder to encrypt/decrypt data	
	private byte[] 	emptyClientId = new byte[32];
	
	private byte[] 	replyClientId = new byte[32];
	private byte 	replyKeyId = 0;		// security key id
	private int		replySeqNum = 0;	// sequence number
	private byte	replyReplyStatus = 0;// status of reply. 
	private byte 	replyError = 0;		// is server reply an error	
	private byte[] 	reply = null;		// reply from server

	public ComRequestBuilder(final String id) throws ComClientException {
		
    	clientId = id.getBytes();
    	if(clientId.length != 32)
    		throw new ComClientException("Invalid client id len. Expected 32, got " + clientId.length);
    	
    	new Random(System.currentTimeMillis()).nextBytes(emptyClientId);
	}
	
	private byte[] createNotEncrypted(int seqNum, byte keyId, short cmdId, final String cmd) throws ComClientException {
	
		if(cmd == null)
			throw new ComClientException("Assert(cmd == null). 1");

		try {		
	    	byte encr = (byte)0;
	    	int outLen = 0;
			byte[] cmdBytes = cmd.getBytes("UTF-8");
			int cmdBytesLen = cmdBytes.length;    	
	    	ByteBuffer bb = null;
	    	outLen = 1		// Is encrypted.
					+ 1 	// Encryption key id length. Bogus value.
					+ 2		// Length of command id.
					+ 4		// Length of sequence number
					+ 1		// Reply status
					+ 32	// Client id
					+ 4		// Length of cmd 
	    			+ cmdBytesLen;
		
			bb = ByteBuffer.allocate(outLen);
			bb.order(ByteOrder.LITTLE_ENDIAN);
			bb.put(encr);
			bb.put(keyId);
			bb.putShort(cmdId);
			bb.putInt(seqNum);
			bb.put((byte)1);
			bb.put(emptyClientId);
			bb.putInt(cmdBytesLen);
			bb.put(cmdBytes);
			
			byte[] out = bb.array();
			return out;
		}
		catch(Exception e) {
			throw new ComClientException(e.getMessage());
		}
	}
	
	public byte[] Create(int seqNum, byte keyId, short cmdId, final String cmd) throws ComClientException {

		if(cmd == null)
			throw new ComClientException("Assert(cmd == null). 2");

		replyError = 0;
		reply = null;
		
		boolean noEncr = cmdId == ComClient.cmdHandShake || cmdId == ComClient.cmdConnect;		
		
		if(coder == null && noEncr == false) 
			throw new ComClientException("Client not connected.");
		
		if(noEncr)
			return createNotEncrypted(seqNum, keyId, cmdId, cmd);
		
		byte[] data = null;
		int dataLen = 0;

		// Step 1. Encrypt values. 
		try {
			if(coder == null)
				throw new ComClientException("Coder not created.");

			byte[] cmdBytes = cmd.getBytes("UTF-8");
			int cmdBytesLen = cmdBytes.length;
			int bufEncrLen = 2		// Command id length
							+ 4		// Sequence number
							+ 1		// Reply status
							+ 32	// Client id length
							+ 4		// Command length as int(0)
							+ cmdBytesLen; 
			
			ByteBuffer bufToEncr = ByteBuffer.allocate(bufEncrLen);
			bufToEncr.order(ByteOrder.LITTLE_ENDIAN);
			bufToEncr.putShort(cmdId);
			bufToEncr.putInt(seqNum);
			bufToEncr.put((byte)1);
			bufToEncr.put(clientId);
			bufToEncr.putInt(cmdBytesLen);
			bufToEncr.put(cmdBytes);
			
	    	try {
    			data = coder.encrypt(bufToEncr.array());
    			dataLen = data.length;
	    	}
	    	catch(Exception e) {
	    		throw new ComClientException("Failed to encrypt request. " + e.getMessage());
	    	}
		}
		catch(UnsupportedEncodingException e) {
			throw new ComClientException(e.getMessage());
		}
		
		// Step 2. Add encrypted and not encrypted values. 
    	byte encr = (byte)1;
    	ByteBuffer bb = null;
    	int outLen = 1				// Is encrypted.
				   + 1 				// Encryption key id length.
				   + 4				// Length of encrypted data.
				   + data.length;	// Length of encrypted part.    		
	
		bb = ByteBuffer.allocate(outLen);
		bb.order(ByteOrder.LITTLE_ENDIAN);
		bb.put(encr);
		bb.put(keyId);
		bb.putInt(dataLen);
		bb.put(data);
    	
    	byte[] out = bb.array();		
    	return out;
	}
	
    public void Parse(final byte[] in) throws ComClientException {

    	ByteBuffer bb = ByteBuffer.wrap(in);
    	bb.order(ByteOrder.LITTLE_ENDIAN);
    	
    	byte encr = bb.get();
    	replyKeyId = bb.get();
    	cmdId = bb.getShort();
    	replySeqNum = bb.getInt();
    	replyReplyStatus = bb.get();
    	bb.get(replyClientId);
    	
    	int len = bb.getInt();
    	replyError = bb.get();
    	
    	if(encr == 1) {
        	if(len == 0)
        		reply = null;
        	else {
        		byte[] enc = new byte[len];
        		bb.get(enc);
        		
        		try {
        			if(coder == null)
        				throw new ComClientException("Coder not created.");
					reply = coder.decrypt(enc);
				} 
        		catch (Exception e) {
					throw new ComClientException("Failed to decrypt reply. " + e.getMessage());
				}
        	}
    	}
    	else {
        	if(len == 0)
        		reply = null;
        	else
        		reply = new byte[len];
        	
        	if(reply != null)
        		bb.get(reply);    		
    	}
    }
    
    public final byte[] getReply() {
    	return reply;
    }
    
    public final String getReplyAsString() throws ComClientException {
    	
    	try {
	    	if(reply != null)
	    		return new String(reply, "UTF-8");
	    	return "";
    	}
    	catch(UnsupportedEncodingException e) {
    		throw new ComClientException(e.getMessage());
    	}
    }
    
	public short getCmdId() {
		return cmdId;
	}
	
	public boolean isReplyError() {
		return replyError == 1;
	}
	
	public void setKey(final String key) {

		coder = new Coder();
		coder.setKey(key);
	}

	public byte getReplyKeyId() {
		return replyKeyId;
	}
		
	public String getReplyClientId() {
		return new String(replyClientId);
	}
	
	public int getReplySeqNum() {
		return replySeqNum;
	}
	
	public char getReplyStatus() {
		return (char)replyReplyStatus;
	}
}
