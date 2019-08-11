#include "DataManager.h"

void SqliteManager::Open(const std::string& path) {
	int ret = sqlite3_open(path.c_str(), &_db);

	//打开成功返回:SQLITE_OK
	if (ret != SQLITE_OK) {
		ERROE_LOG("sqlite3_open\n");
	}
	else {   //打开成功
		TRACE_LOG("sqlite3_open\n");
	}
}

void SqliteManager::Close() {
	int ret = sqlite3_close(_db);

	if (ret != SQLITE_OK) {
		ERROE_LOG("sqlite3_close error\n");
	}
	else {   //打开成功
		TRACE_LOG("sqlite3_close success\n");
	}
}

void SqliteManager::ExecuteSql(const std::string& sql) {
	char* errmsg;
	int ret = sqlite3_exec(_db, sql.c_str(), nullptr, nullptr, &errmsg);
	//第三个参数表示回调函数，如果不需要操作就赋值为空

	if (ret != SQLITE_OK) {
		ERROE_LOG("sqlite3_exec(%s) errmsg:(%s)\n", sql.c_str(), errmsg);
		sqlite3_free(errmsg);
	}
	else {   //打开成功
		TRACE_LOG("sqlite3_exec(%s) success\n", sql.c_str());
	}
}

void SqliteManager::GetTable(const std::string& sql, int& row, int &col, char**& ppRet) {
	assert(_db);	//_db如果为空，说明数据库没有初始化

	//最后一个参数char的原因是：文件只有文本文件与二进制文件格式，所以选择字符型。
	char* errmsg;
	int ret = sqlite3_get_table(_db, sql.c_str(), &ppRet, &row, &col, &errmsg);

	if (ret != SQLITE_OK) {
		ERROE_LOG("sqlite3_get_table(%s) errmsg:(%s)\n", sql.c_str(), errmsg);
		sqlite3_free(errmsg);
	}
	else {
		TRACE_LOG("sqlite3_get_table(%s) success\n", sql.c_str());
	}
}


///////////////////////////////////////////////////////////////////
void DataManager::Init() {
	//打开数据库连接
	std::unique_lock<std::mutex> lock(_mtx);
	_dbmgr.Open(DB_NAME);
	//建表
	char sql[256];
	//sprintf 序列化到字符串中
	sprintf(sql, "create table if not exists %s (	\
			id INTEGER PRIMARY KEY, \
			path text,	\
			name text,\
			name_pinyin text,	\
			name_initials)", \
		TB_NAME
	);
	_dbmgr.ExecuteSql(sql);
}

void DataManager::GetDocs(const string& path, std::set<string>& dbset) {
	char sql[256];
	sprintf(sql, "select name from %s where path = '%s'", TB_NAME, path.c_str());
	int row, col;
	char** ppRet;


	//lock_guard():把所对象交给它，在它的构造函数中加锁，出了作用域自动解锁
	//				只有构造和析构，只负责RAII，不能主动释放
	//unique_lock():RAII的同时，用户可以自动加锁解锁，主动释放
	std::unique_lock<std::mutex> lock(_mtx);		//处理下一句指令：抛异常 的可能
	AutoGetTable agt(_dbmgr, sql, row, col, ppRet);	//如果抛异常，lock自动解锁
	lock.unlock();		//执行到这里说明没有异常抛出，主动解锁，析构函数就不需要再解锁了

	for (int i = 1; i <= row; ++i) {	//包含= 因为一列为表名
		for (int j = 0; j < col; ++j) {
			dbset.insert(ppRet[i*col + j]);		//添加进set中
		}
	}
}

void DataManager::InsertDoc(const string& path, const string& name) {
	char sql[256];
	string pinyin = ChineseConvertPinYinAllSpell(name);
	string initials = ChineseConvertPinYinInitials(name);
	sprintf(sql, "insert into %s(path,name,name_pinyin,name_initials) \
		values('%s','%s','%s','%s')", TB_NAME, path.c_str(), name.c_str(), pinyin.c_str(), initials.c_str());
	std::unique_lock<std::mutex> lock(_mtx);
	_dbmgr.ExecuteSql(sql);
}

void DataManager::DeleteDoc(const string& path, const string& name) {
	char sql[256];

	//1. 删目录
	sprintf(sql, "delete from %s where path = '%s' and name = '%s'", TB_NAME, path.c_str(), name.c_str());
	_dbmgr.ExecuteSql(sql);

	//2. 不光删除本文档，还要删除文档的< 子文档 >
	string _path = path;
	_path += '\\';
	_path += name;
	sprintf(sql, "delete from %s where path like '%s%%'", TB_NAME, _path.c_str());

	std::unique_lock<std::mutex> lock(_mtx);
	_dbmgr.ExecuteSql(sql);
}

void DataManager::Search(const string& key, std::vector<std::pair<string, string>>& docinfos) {
	char sql[256];

	//1. 拼音搜索
	string pinyin = ChineseConvertPinYinAllSpell(key);
	//2. 首字母搜索
	string initials = ChineseConvertPinYinInitials(key);

	//满足两条件任一即可
	sprintf(sql, "select name,path from %s where name_pinyin like '%%%s%%' or name_initials like '%%%s%%'", TB_NAME, pinyin.c_str(), initials.c_str());

	int row, col;
	char** ppRet;

	std::unique_lock<std::mutex> lock(_mtx);
	AutoGetTable agt(_dbmgr, sql, row, col, ppRet);
	lock.unlock();

	for (int i = 1; i <= row; ++i) {	//包含= 因为一列为表名
		docinfos.push_back(std::make_pair(ppRet[i*col + 0], ppRet[i*col + 1]));
	}
}

void DataManager::SpliteHighlight(const string& str, const string& key, string& prefix, string& highlight, string& suffix) {

	// 1. key是原串子串
	{
		size_t highlight_start = str.find(key);
		if (highlight_start != string::npos) {
			prefix = str.substr(0, highlight_start);
			highlight = key;
			suffix = str.substr(highlight_start + key.size(), string::npos);
			return;		//但凡有任一种情况匹配上了，就直接跳出
		}
	}

	// 2. key的拼音全拼:用拼音去匹配拼音
	{
		string key_allspell = ChineseConvertPinYinAllSpell(key);
		string str_allspell = ChineseConvertPinYinAllSpell(str);
		size_t highlight_index = 0;
		size_t allspell_index = 0;
		size_t highlight_start = 0, highlight_len = 0;

		size_t allspell_start = str_allspell.find(key_allspell);

		if (allspell_index != string::npos) {

			size_t allspell_end = allspell_start + key_allspell.size();
			while (allspell_index < allspell_start) {
				if (allspell_index == allspell_start) {
					highlight_start = highlight_index;
				}

				//如果是ascii字符，跳过
				if (str[highlight_index >= 0 && str[highlight_index] <= 127]) {
					++highlight_index;
					++allspell_index;
				}
				else {		//汉字
					char chinese[3] = { 0 };
					chinese[0] = str[highlight_index];
					chinese[1] = str[highlight_index + 1];
					string allspell_str = ChineseConvertPinYinAllSpell(chinese);

					highlight_index += 2;	//gbk汉字是两个字符
					allspell_index += allspell_str.size();
				}
			}
			highlight_len = highlight_index - highlight_start;

			prefix = str.substr(0, highlight_start);
			highlight = str.substr(highlight_start, highlight_len);
			suffix = str.substr(highlight_start + highlight_len, string::npos);
			return;
		}
	}

	// 3. key的拼音首字母
	{
		string initial_str = ChineseConvertPinYinInitials(str);
		string initial_key = ChineseConvertPinYinInitials(key);
		size_t initial_start = initial_str.find(initial_key);
		if (initial_start != string::npos) {
			size_t initial_end = initial_start + initial_key.size();
			size_t initial_index = 0, highlight_index = 0;
			size_t highlight_start = 0, highlight_len = 0;
			while (initial_index < initial_end) {
				if (initial_index == initial_start) {
					highlight_start = highlight_index;
				}
				//ascii字符
				if (str[highlight_index] >= 0 && str[highlight_index] <= 127) {
					++highlight_index;
					++initial_index;
				}
				else {
					highlight_index += 2;	//汉字走一个
					++initial_index;	//首字母走一个
				}
			}
			highlight_len = highlight_index - highlight_start;

			prefix = str.substr(0, highlight_start);
			highlight = str.substr(highlight_start, highlight_len);
			suffix = str.substr(highlight_start + highlight_len, string::npos);
			return;
		}
	}
	TRACE_LOG("Splite Highlight not match. str:(%s)\n", str.c_str(), key.c_str());
	prefix = str;	//保险：没有匹配上，不高亮显示
}
