#include "globals.h"
#include "sqlite3.h"
#include "network.h"
#include "router.h"
#include "db.h"
sqlite3 *db;
int rc;

int db_callback_got_host(void *NotUsed, int argc, char **argv, 
                    char **azColName)
{
	callback_got_host(argv[0], argv[1]);
	return 0;
}

int db_count=0;
int db_callback_got_count(void *NotUsed, int argc, char **argv, char **azColName)
{
	sscanf(argv[0], "%d", &db_count);
	return 0;
}
void db_add_host(char* IP, int Port)
{
	char SQL[1024];
	db_count = 0;
	sprintf(SQL, "SELECT COUNT(*) FROM hosts WHERE IP='%s' AND Port=%u", IP, Port);
	rc = sqlite3_exec(db, SQL, db_callback_got_count, 0, 0);
	if (db_count == 0)
	{
		sprintf(SQL, "INSERT OR IGNORE INTO hosts(IP, Port)VALUES('%s', %u)", IP, Port);
		rc = sqlite3_exec(db, SQL, 0, 0, 0);
		if (rc != SQLITE_OK) {
			fprintf(stderr, "Failed to insert: %s\n", sqlite3_errmsg(db));
			exit(-1);
		}
		sprintf(SQL, "SELECT IP, Port FROM hosts WHERE Key='' AND IP='%s' AND Port=%u", IP, Port);
		rc = sqlite3_exec(db, SQL, db_callback_got_host, 0, 0); 
		if (rc != SQLITE_OK) {
			fprintf(stderr, "Failed to select: %s\n", sqlite3_errmsg(db));
			exit(-1);
		}
		db_sync();
	}
}
void db_add_ethernet(char* MAC, char* Key)
{
	if (Debug)
		printf("Adding MAC %s via %s\n", MAC, Key);
	char SQL[1024];
	db_count = 0;
	sprintf(SQL, "SELECT COUNT(*) FROM macs WHERE MAC='%s' AND Key='%s'", MAC, Key);
	if (Debug>1)
		printf("SQL: %s\n", SQL);
	rc = sqlite3_exec(db, SQL, db_callback_got_count, 0, 0);
	if (db_count == 0)
	{
		sprintf(SQL, "INSERT OR REPLACE INTO macs(MAC, Key)VALUES('%s', '%s')", MAC, Key);
		if (Debug>1)
			printf("SQL: %s\n", SQL);
		rc = sqlite3_exec(db, SQL, 0, 0, 0);
		if (rc != SQLITE_OK) {
			fprintf(stderr, "Failed to insert: %s\n", sqlite3_errmsg(db));
			exit(-1);
		}
		db_sync();
	}
}


void db_set_node(char* IP, int Port, char* remote_key, char* IPv4, char*IPv6)
{
	char SQL[1024];
	db_count = 0;
	sprintf(SQL, "SELECT COUNT(*) FROM hosts WHERE Key='%s'", remote_key);
	if (Debug>1)
		printf("SQL: %s\n", SQL);
	rc = sqlite3_exec(db, SQL, db_callback_got_count, 0, 0);
	if (db_count == 0)
	{
		sprintf(SQL, "UPDATE hosts SET Key='%s', IPv4='%s', IPv6='%s', State='OK', Retries=0, LastRecv=%u WHERE IP='%s' AND Port=%u", remote_key, IPv4, IPv6, time(NULL), IP, Port);
		if (Debug>1)
			printf("%s\n", SQL);
		rc = sqlite3_exec(db, SQL, NULL, 0, 0);
		db_sync();
		if (Debug)
			printf("Set key %s for %s %u\n", remote_key, IP, Port);
		update_router();
	}
}
int db_callback_got_node(void *NotUsed, int argc, char **argv, 
                    char **azColName)
{
	struct Node* Node = (struct Node*) malloc(sizeof(struct Node));
	memset(Node, 0, sizeof(struct Node));
	Node->address.sin6_family = AF_INET6;
	if (inet_pton(AF_INET6, argv[0], &Node->address.sin6_addr) == 1 && atoi(argv[1]))
	{
		Node->address.sin6_port = htons(atoi(argv[1]));
		sscanf(argv[2], "%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx", &Node->Key[0], &Node->Key[1], &Node->Key[2], &Node->Key[3], &Node->Key[4], &Node->Key[5]);
		inet_pton(AF_INET, argv[3], &Node->IPv4);
		inet_pton(AF_INET6, argv[4], &Node->IPv6);
		Node->LastSend = atol(argv[5]);
		Node->LastRecv = atol(argv[6]);
		Node->Next = NodeList;
		if (Debug>1)
			printf("Sync Node %s:%s:%s\n", argv[0], argv[1], argv[2]);
		NodeList = Node;
		return 0;
	}
	return 1;
}
int db_callback_got_mac(void *NotUsed, int argc, char **argv, 
                    char **azColName)
{
	unsigned char MAC[sizeof(node_key)];
	sscanf(argv[0], "%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx", &MAC[0], &MAC[1], &MAC[2], &MAC[3], &MAC[4], &MAC[5]);
	unsigned char Key[sizeof(node_key)];
	sscanf(argv[1], "%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx", &Key[0], &Key[1], &Key[2], &Key[3], &Key[4], &Key[5]);
	
	struct Node* Node = NodeList;
	while (Node)
	{
		if (memcmp(Node->Key, Key, sizeof(node_key)) == 0)
		{
			struct EthernetNode* EthernetNode = (struct EthernetNode*) malloc(sizeof(struct EthernetNode));
			memset(EthernetNode, 0, sizeof(struct EthernetNode));
			memcpy(EthernetNode->MAC, MAC, 6);
			EthernetNode->Node = Node;
			EthernetNode->Next = EthernetNodeList;
			EthernetNodeList = EthernetNode;
			if (Debug>1)
				printf("Sync MAC %s:%s\n", argv[0], argv[1]);
			return 0;
		}
		Node=Node->Next;
	}
	if (Debug)
		printf("Sync MAC %s:%s FAILED!\n", argv[0], argv[1]);
	return 0;
}
void db_sync(void)
{
	char SQL[1024];
	if (Debug>1)
		printf("SYNC: Clear Nodes\n");
	if (Debug>1)
		printf("SQL: BEGIN\n");
	sqlite3_exec(db, "BEGIN", NULL, 0, 0);
	while (NodeList)
	{
		struct Node* Next = NodeList->Next;
		char IP[72];
		inet_ntop(AF_INET6, &NodeList->address.sin6_addr, IP, sizeof(IP));
		if (NodeList->LastSend < time(NULL)-30)
			network_ping(NodeList);
		sprintf(SQL, "UPDATE hosts SET LastSend=%u,LastRecv=%u WHERE IP='%s' AND Port=%u AND State='OK'", (unsigned long)NodeList->LastSend, (unsigned long)NodeList->LastRecv, IP, htons(NodeList->address.sin6_port));
		if (Debug>1)
			printf("SQL: %s\n", SQL);
		sqlite3_exec(db, SQL, NULL, 0, 0);
		free(NodeList);
		NodeList=Next;
	}
	sprintf(SQL, "UPDATE hosts SET State='INIT',Retries=0,Key='',IPv4='',IPv6='',LastSend=0,LastRecv=0 WHERE LastRecv < %u AND NOT LastRecv = 0", (unsigned long)time(NULL)-90);
	if (Debug>1)
		printf("SQL: %s\n", SQL);
	sqlite3_exec(db, SQL, NULL, 0, 0);   
	if (Debug>1)
		printf("SYNC: Clear MACs\n");
	while (EthernetNodeList)
	{
		struct EthernetNode* Next = EthernetNodeList->Next;
		free(EthernetNodeList);
		EthernetNodeList=Next;
	}
	
	sprintf(SQL, "SELECT IP, Port, Key, IPv4, IPv6, LastSend, LastRecv FROM hosts WHERE NOT Key = ''");
	if (Debug>1)
		printf("SQL: %s\n", SQL);
	rc = sqlite3_exec(db, SQL, db_callback_got_node, 0, 0);    
	if (rc != SQLITE_OK) {
		fprintf(stderr, "Failed to sync nodes: %s\n", sqlite3_errmsg(db));
		exit(-1);
	}
	sprintf(SQL, "DELETE FROM macs WHERE Key IN (select macs.Key from macs LEFT OUTER JOIN hosts ON macs.Key=hosts.Key WHERE hosts.Key IS NULL)");
	if (Debug>1)
		printf("SQL: %s\n", SQL);
	rc = sqlite3_exec(db, SQL, db_callback_got_mac, 0, 0);    
	sprintf(SQL, "SELECT MAC, Key FROM macs");
	if (Debug>1)
		printf("SQL: %s\n", SQL);
	rc = sqlite3_exec(db, SQL, db_callback_got_mac, 0, 0);    
	if (rc != SQLITE_OK) {
		fprintf(stderr, "Failed to sync nodes: %s\n", sqlite3_errmsg(db));
		exit(-1);
	}
	if (Debug>1)
		printf("SQL: COMMIT\n");
	sqlite3_exec(db, "COMMIT", NULL, 0, 0);
}
void db_sent_init(char* IP, char* Port)
{
	char SQL[1024];
	sprintf(SQL, "UPDATE hosts SET State='INIT', IPv4='', IPv6='', Retries=Retries+1 WHERE IP='%s' AND Port=%s AND State='INIT'", IP, Port);
	if (Debug>1)
		printf("SQL: %s\n", SQL);
	rc = sqlite3_exec(db, SQL, NULL, 0, 0);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "Failed to update retry count: %s\n", sqlite3_errmsg(db));
		exit(-1);
	}
}
int db_send_nodelist(void *NotUsed, int argc, char **argv, 
                    char **azColName)
{
	unsigned char Key[sizeof(node_key)];
	sscanf(argv[0], "%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx", &Key[0], &Key[1], &Key[2], &Key[3], &Key[4], &Key[5]);
	
	struct Node* Node = NodeList;
	while (Node)
	{
		if (memcmp(Node->Key, Key, sizeof(node_key)) == 0)
		{
			network_send_nodelist(Node);
			char SQL[1024];
			sprintf(SQL, "UPDATE hosts SET LastSync=%u WHERE Key='%s'", (unsigned long)time(NULL), argv[0]);
			if (Debug>1)
				printf("SQL: %s\n", SQL);
			rc = sqlite3_exec(db, SQL, db_send_nodelist, 0, 0);
			if (rc != SQLITE_OK) {
				fprintf(stderr, "Failed to update node sync time: %s\n", sqlite3_errmsg(db));
				exit(-1);
			}
			return 0;
		}
		Node=Node->Next;
	}
}
void db_init_hosts()
{
	char SQL[1024];
	sprintf(SQL, "SELECT IP, Port FROM hosts WHERE State='INIT'");
	if (Debug>1)
		printf("SQL: %s\n", SQL);
	rc = sqlite3_exec(db, SQL, db_callback_got_host, 0, 0);    
	if (rc != SQLITE_OK) {
		fprintf(stderr, "Failed to select: %s\n", sqlite3_errmsg(db));
		exit(-1);
	}    

	sprintf(SQL, "SELECT Key FROM hosts WHERE State='OK' AND LastSync < %u", (unsigned long)time(NULL) - 300);
	if (Debug>1)
		printf("SQL: %s\n", SQL);
	rc = sqlite3_exec(db, SQL, db_send_nodelist, 0, 0);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "Failed to send nodelist: %s\n", sqlite3_errmsg(db));
		exit(-1);
	}
}
void db_clear_dead_hosts()
{
	char SQL[1024];
	if (Debug>1)
		printf("Clear dead nodes\n");
	sprintf(SQL, "DELETE FROM hosts WHERE State='INIT' AND Retries > 5");
	if (Debug>1)
		printf("SQL: %s\n", SQL);
	rc = sqlite3_exec(db, SQL, NULL, 0, 0);    
	sprintf(SQL, "DELETE FROM hosts WHERE State='INIT' AND Retries > 5");
	if (Debug>1)
		printf("SQL: %s\n", SQL);
	rc = sqlite3_exec(db, SQL, NULL, 0, 0);    
}
void db_init(char* filename)
{
	rc = sqlite3_open(filename[0]?filename:":memory:", &db);
	sqlite3_stmt *res;
	char err[1024]={0};
    
	if (rc != SQLITE_OK) {
		fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
		exit(-1);
	}
    
	rc = sqlite3_exec(db, "CREATE TABLE hosts(IP, Port, State DEFAULT 'INIT', Retries DEFAULT 0, Key, IPv4, IPv6, LastSend DEFAULT 0, LastRecv DEFAULT 0, LastSync DEFAULT 0)", 0, 0, 0);
	rc = sqlite3_exec(db, "UPDATE hosts SET Key='', State='INIT', IPv4='', IPv6='', LastSend=0, LastRecv=0", 0, 0, 0);
	rc = sqlite3_exec(db, "CREATE UNIQUE INDEX hosts_idx on hosts(IP, Port)", 0, 0, 0);
	rc = sqlite3_exec(db, "CREATE TABLE macs(MAC, Key)", 0, 0, 0);
	rc = sqlite3_exec(db, "CREATE UNIQUE INDEX macs_idx on macs(MAC, Key)", 0, 0, 0);
	rc = sqlite3_exec(db, "DELETE FROM macs", 0, 0, 0);
}
