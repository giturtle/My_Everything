#pragma once

#include "Common.h"

class SqliteManager {
public:
	SqliteManager()
		:_db(nullptr)
	{}

	~SqliteManager() {
		Close();
	}

	void Open(const std::string& path);	//打开，操作数据库文件
	void Close();	//希望随着对象析构函数调用
	void ExecuteSql(const std::string& sql);	//执行sql语句，如果执行错误则报错，通过日志打印哪一句出错的，发生的什么错误
	void GetTable(const std::string& sql, int& row, int &col,char**& ppRet);	//sqlite查询返回的是一个< 二维数组 >
																//这个数组还是动态开辟的，所以选择使用RAII思想释放该内存

	//禁用拷贝构造 与 赋值
	SqliteManager(const SqliteManager&) = delete;
	SqliteManager& operator=(const SqliteManager&) = delete;

private:
	sqlite3* _db;	//数据库对象
};


//RAII
class AutoGetTable {	//除了作用域自动释放内存
public:
	AutoGetTable(SqliteManager& sm,const std::string& sql, int& row, int &col, char**& ppRet) {
		sm.GetTable(sql, row, col, ppRet);
		_ppRet = ppRet;	//托管
	}

	~AutoGetTable() {
		sqlite3_free_table(_ppRet);
	}
	//借鉴unique_ptr
	AutoGetTable(const AutoGetTable&) = delete;
	AutoGetTable operator=(const AutoGetTable&) = delete;
private:
	char** _ppRet;
};



/////////////////////////////////////////////////////////////
//	数据库管理模块

#define TB_NAME "tb_doc"
#define DB_NAME "doc.db"

//为了方便加锁，设计为单例
class DataManager {
public:
	static DataManager* GetInstance() {
		//饿汉模式
		static DataManager datamgr;
		datamgr.Init();
		return &datamgr;
	}
	void Init();		//建表，打开数据库
	
	void GetDocs(const string& path, std::set<string>& dbset);//查找path下的所有子文档
	void InsertDoc(const string& path, const string& doc);
	void DeleteDoc(const string& path, const string& doc);
	void Search(const string& key, std::vector<std::pair<string, string>>& docinfos);
	void SpliteHighlight(const string& std, const string& key, string& prefix, string& highlight, string& suffix);
private:
	DataManager()
	{}

	SqliteManager _dbmgr;

	//加锁
	std::mutex _mtx;
};
