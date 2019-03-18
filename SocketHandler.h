class SocketHandler {
public:
	unsigned char* WriteBuffer=NULL;
	int WriteBufferLength = 0;
	virtual int GetHandle() {}
	virtual bool ReadHandle() {}
	virtual void WriteHandle() {}
};
