package com.zs.client.client;

import java.security.KeyFactory;
import java.security.PublicKey;
import java.security.spec.X509EncodedKeySpec;
import java.util.Base64;

import javax.crypto.Cipher;

/**
 * Class to encrypt string with RSA public key
 */
public class CoderRSA {

	static private PublicKey getPublicKey(final String key) throws Exception {
		
		byte[] byteKey = Base64.getDecoder().decode(key.getBytes());
		X509EncodedKeySpec spec = new X509EncodedKeySpec(byteKey);
		KeyFactory kf = KeyFactory.getInstance("RSA");
		return kf.generatePublic(spec);
	}

	static public String Encrypt(final String input, final String publicKey) throws Exception {
		
		PublicKey pk = getPublicKey(publicKey);
    	Cipher cipher = Cipher.getInstance("RSA/ECB/PKCS1Padding");
    	cipher.init(Cipher.ENCRYPT_MODE, pk);

    	byte[] cipherData = cipher.doFinal(input.getBytes());
    	
    	StringBuffer sb = new StringBuffer();
    	for(int i = 0; i < cipherData.length; ++i)
    		sb.append(String.format("%02X", cipherData[i]));
    	
    	return sb.toString();
	}
}
