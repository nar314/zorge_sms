package com.zt.client;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

/**
 * Unit test for simple App.
 */
public class AppTest extends TestCase
{
    public AppTest( String testName ){
        super( testName );
    }

    public static Test suite() {
        return new TestSuite( AppTest.class );
    }

    public void testApp() {
/*    	
    	try {
        	ComClient c = new ComClient("", 1);
        	String s = c.newGuid();
        	System.out.println("[" + s + "]");
        	System.out.println(s.length());
    	}
    	catch(Exception e) {
    		System.out.println(e.getMessage());
    	}
*/
/*
    	try {
    		DlgOpenNumber dlg = new DlgOpenNumber(null);
    		dlg.showDialog();
    	}
    	catch(Exception e) {
    		System.out.println(e.getMessage());
    	}
*/
    }
}
