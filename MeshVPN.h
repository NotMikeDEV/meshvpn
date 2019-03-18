class MeshVPN {
private:
	std::vector<SocketHandler*> SocketList;
	TunTap* Interface;
	Switch* LocalSwitch;
public:
	void AddSocket(SocketHandler* Handler);
	void DelSocket(SocketHandler* Handler);
	bool Running;
	MeshVPN();
	~MeshVPN();
	void Init(Switch* Switch);
	void LoadNodes(char* Filename);
	void SaveNodes(char* Filename);
	bool Loop();
};
