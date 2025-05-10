package com.zs.client;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.util.ArrayList;

public class ConfigLoader {

	private String root = null;
	
	public ConfigLoader() throws Exception {
		root = getFolder();
	}
	
	public ArrayList<Config> loadConfigs() throws Exception {
	
		ArrayList<Config> out = new ArrayList<Config>();
		File dir = new File(root);
		for(File f : dir.listFiles()) {
			if(!f.isFile())
				continue; // not a file
			String absPath = f.getAbsolutePath();
			if(!absPath.endsWith(".config"))
				continue; // not config
			
			try (FileInputStream stream = new FileInputStream(absPath)) {
				String content = new String(stream.readAllBytes());
				
				try {
					Config c = Config.create(content);
					out.add(c);
				}
				catch(Exception e) {
					System.out.println(e.getMessage());
				}
			}
		}
		return out;
	}
	
	public void storeConfig(final Config config) throws Exception {
		
		String fullPath = root + config.getName() + ".config";
		try (FileOutputStream stream = new FileOutputStream(fullPath)) {
		    stream.write(config.toString().getBytes());
		}
		System.out.println("Config stored in " + fullPath);
	}
	
	private String getFolder() throws Exception {

		String folder = ConfigLoader.class.getProtectionDomain().getCodeSource().getLocation().getPath().toString();
		if(folder.endsWith("/") || folder.endsWith("\\"))
			folder = folder.substring(0, folder.length() - 1);
		
		int pos = folder.lastIndexOf('/');
		if(pos == -1)
			pos = folder.lastIndexOf('\\');
		
		if(pos == -1)
			throw new Exception("Can not get current folder.");
		
		folder = folder.substring(0, pos);		
		if(!folder.endsWith("/") && !folder.endsWith("\\"))
			folder += "/";

		File f = new File(folder);
		if(!f.exists())
			throw new Exception("Current folder does not exist. " + folder);
		if(!f.isDirectory())
			throw new Exception("Current folder is not directory. " + folder);
		
		return folder;
	}	
}
