void callback_got_host(char* IP, char* Port);
void db_init(char* filename);
void db_add_host(char* IP, int Port);
void db_add_ethernet(char* MAC, char* Key);
void db_set_node(char* IP, int Port, char* remote_key, char* IPv4, char*IPv6);
void db_sent_init(char* IP, char* Port);
void db_init_hosts(void);
void db_clear_dead_hosts(void);
void db_sync(void);