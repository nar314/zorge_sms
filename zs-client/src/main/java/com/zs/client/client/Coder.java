package com.zs.client.client;

import java.security.MessageDigest;
import java.util.HashMap;

import javax.crypto.Cipher;
import javax.crypto.spec.SecretKeySpec;
import javax.crypto.spec.IvParameterSpec;

public class Coder {

	private HashMap<String, byte[]> md5s = new HashMap<String, byte[]>();
	private String key = null;
	
	private byte[] calcMD5(final String s) throws Exception {
	
		if(md5s.size() > 64)
			md5s.clear();

		byte[] out = md5s.get(s);
		if(out != null)
			return out;
		
		byte[] input = s.getBytes("UTF-8");
		MessageDigest md = MessageDigest.getInstance("MD5");
		out = md.digest(input);
		if(out.length != 16)
			throw new Exception("Unexpected len = " + out.length + ", expected 16");
		md5s.put(s, out);
		return out;
	}

	//------------------------------------------------------------------------------
	//
	//------------------------------------------------------------------------------
	public void setKey(final String key) {
		this.key = key;
	}

	//------------------------------------------------------------------------------
	//
	//------------------------------------------------------------------------------	
	public byte[] encrypt(byte[] plainBytes) throws Exception {
		
		if(key == null || key.length() == 0)
			throw new Exception("Key not set");
		
        byte[] iv = new byte[16];
        
        byte[] md5 = calcMD5(key);
        for(int i = 0; i < 16; ++i)
        	iv[i] = md5[i];
        
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
        return encrypted;
    }

	//------------------------------------------------------------------------------
	//
	//------------------------------------------------------------------------------	
	public byte[] decrypt(byte[] encryptedBytes) throws Exception {
		
		if(key == null || key.length() == 0)
			throw new Exception("Key not set");

        byte[] iv = new byte[16];
        byte[] md5 = calcMD5(key);
        for(int i = 0; i < 16; ++i)
        	iv[i] = md5[i];
        
        IvParameterSpec ivParameterSpec = new IvParameterSpec(iv);
        byte[] keyBytes = new byte[32];
        for(int i = 0; i < 16; ++i)
        	keyBytes[i] = iv[i];
        for(int i = 16; i < 32; ++i)
        	keyBytes[i] = iv[i - 16];

        SecretKeySpec secretKeySpec = new SecretKeySpec(keyBytes, "AES");
        Cipher cipherDecrypt = Cipher.getInstance("AES/CBC/PKCS5Padding");
        cipherDecrypt.init(Cipher.DECRYPT_MODE, secretKeySpec, ivParameterSpec);
        byte[] decrypted = cipherDecrypt.doFinal(encryptedBytes);
        return decrypted;
    }
}
