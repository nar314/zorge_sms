package com.zt.client;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.math.BigInteger;
import java.nio.charset.StandardCharsets;
import java.security.KeyFactory;
import java.security.PrivateKey;
import java.security.PublicKey;
import java.security.interfaces.RSAPublicKey;
import java.security.spec.PKCS8EncodedKeySpec;
import java.security.spec.RSAPublicKeySpec;
import java.security.spec.X509EncodedKeySpec;
import java.util.ArrayList;
import java.util.Base64;
import java.util.Base64.Decoder;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javax.crypto.Cipher;

//import java.util.UUID;

import org.junit.jupiter.api.Test;

public class AppTest {

	
	private static PublicKey getPublicKey(String key) throws Exception {
		
		byte[] byteKey = Base64.getDecoder().decode(key.getBytes());
		X509EncodedKeySpec spec = new X509EncodedKeySpec(byteKey);
		KeyFactory kf = KeyFactory.getInstance("RSA");
		return kf.generatePublic(spec);
	}

	private static PrivateKey getPrivateKey(String key) throws Exception {
		
		byte[] byteKey = Base64.getDecoder().decode(key.getBytes());
		X509EncodedKeySpec spec = new X509EncodedKeySpec(byteKey);
		KeyFactory kf = KeyFactory.getInstance("RSA");
		//return kf.generatePublic(spec);
		return kf.generatePrivate(spec);
	}

    @Test
    public void F1() throws Exception {
    	if(true)
    		return;
    	// openssl x509 -pubkey -in z1.pem
    	// use public key output as input for Java.
    	
    	try {
    		System.out.println("Enter public key:");
			BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));
			String line = reader.readLine();

	    	String publicKey = line;
	    	//publicKey = "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA08kbLB4oRRCJ+WZT0adxq9tTV39vVfjkIoixlPIsKQ9YHIVf0J4GDY0ZpybGqjXBURDxl919AJd21VP3keMb+OeiUjnHeS0DslGDXwHQcuE8POLG2p9rT+YRlO4ZkNVpi8BB/h+pHE6xiYRXKyFI9MsdTYU9DO1DFyy24TkNm2ibGZi1SXceE/hOjH1aDGyfP8palFUJU7iPInoYuIkfByRWvPdK540zMjYDZyNf7IZn7PClnTbWN0MhoivLSmN6E2lMEp5G6wFrzNHMOQnn90D+Y1qVQdjrAC91dazQzNrt6gU6Pxb9nMF20CyImmgLax+EHSJyxgg0IL8LN8vIgQIDAQAB";
	    	
	    	System.out.println("\n[" + publicKey + "]");
	    	PublicKey pk = getPublicKey(publicKey);
	    	
	    	Cipher cipher = Cipher.getInstance("RSA/ECB/PKCS1Padding"); // <- original
	    	cipher.init(Cipher.ENCRYPT_MODE, pk);

	    	String text = "Hello world from Java !";
	    	byte[] cipherData = cipher.doFinal(text.getBytes());
	    	
	    	System.out.println("\nEncrypted data\n------------ begin");
	    	for(int i = 0; i < cipherData.length; i++)
	    		System.out.print(String.format("%02X", cipherData[i]));
	    	System.out.println("\n------------ end");
/*
	    	{
	    		//Cipher c2 = Cipher.getInstance("RSA");
	    		Cipher c2 = Cipher.getInstance("RSA/ECB/PKCS1Padding"); // <- original
	    		PrivateKey prk = getPrivateKey(publicKey);
	    		c2.init(Cipher.DECRYPT_MODE, prk);
	    		byte[] d = c2.doFinal(cipherData);
	    		
	    		String z1 = new String(d, StandardCharsets.UTF_8);
	    		System.out.println(z1);
	    		
		    	System.out.println("\nDecripted data\n------------ begin");
		    	for(int i = 0; i < d.length; i++)
		    		System.out.print(String.format("%02X %c", d[i], d[i]));
		    	System.out.println("\n------------ end");
	    	}
*/
	    	System.out.println("Done.");
    	}
    	catch(Exception e) {
    		System.out.println(e.getMessage());
    	}
    }
}
