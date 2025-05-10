package com.zs.client;

public class Config {

	private String name = null;
	private String server = "localhost";
	private int port = 7530;
	private int timeout = 10; // Communication timeout in seconds
	private int refreshTimeout = 10; // Messages refresh timeout in seconds
	
	public Config() {
		name = java.util.UUID.randomUUID().toString();
	}
	
	public void setServer(final String server) throws Exception {
		
		if(server == null || server.isEmpty())
			throw new Exception("Invalid server name");
		this.server = server;
	}	
	public String getServer() {
		return server;
	}

	public void setPort(int port) throws Exception {
		
		if(port < 1)
			throw new Exception("Invalid port");
		this.port = port;
	}
	public int getPort() {
		return port;
	}

	public void setTimeout(int timeout) throws Exception {
		
		if(timeout < 0)
			throw new Exception("Invalid timeout");
		this.timeout = timeout;
	}
	public int getTimeout() {
		return timeout;
	}
	
	public void setRefreshTimeout(int timeout) throws Exception {
		
		if(timeout < 0)
			throw new Exception("Invalid refresh timeout");
		this.refreshTimeout = timeout;
	}
	public int getRefreshTimeout() {
		return refreshTimeout;
	}
	
	public void clearName() {
		name = "";
	}
	public void setName(final String name) throws Exception {
		
		if(name == null || name.isEmpty())
			throw new Exception("Invalid name");
		this.name = name;
	}	
	public String getName() {
		return name;
	}
	
	public String toString() {
		
		StringBuilder sb = new StringBuilder();
		sb.append("name=").append(name).append("\n");
		sb.append("server=").append(server).append("\n");
		sb.append("port=").append(port).append("\n");
		sb.append("timeout=").append(timeout).append("\n");
		sb.append("refreshTimeout=").append(refreshTimeout).append("\n");
		return sb.toString();
	}
	
	static public Config create(final String content) throws Exception {
		
		Config out = new Config();
		out.clearName();
		
		String[] lines = content.split("\n");
		for(String line : lines) {
			String[] pair = line.split("=");
			if(pair.length != 2)
				throw new Exception("Invalid line : " + line);
			String name = pair[0].trim();
			if(name.equals("name"))
				out.setName(pair[1].trim());
			else if(name.equals("server"))
				out.setServer(pair[1].trim());
			else if(name.equals("port"))
				out.setPort(Integer.parseInt(pair[1].trim()));
			else if(name.equals("timeout"))
				out.setTimeout(Integer.parseInt(pair[1].trim()));
			else if(name.equals("refreshTimeout"))
				out.setRefreshTimeout(Integer.parseInt(pair[1].trim()));						
		}
		
		if(out.getName().isEmpty() || out.getServer().isEmpty() || out.getPort() < 1 || 
				out.getTimeout() < 0 || out.getRefreshTimeout() <= 0)
			throw new Exception("Invalid config");
		
		return out;
	}
}
