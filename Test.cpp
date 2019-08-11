

#include "Common.h"
#include "DataManager.h"
#include "ScanManager.h"

void Test() {
	vector<string> dirs;
	vector<string> files;
	DirectoryList("D:\\Common", dirs, files);
	for (const auto& e : dirs) {
		cout << e << endl;
	}
	/*for (const auto& e : files) {
		cout << e << endl;
	}*/
}
void TestSqlite() {
	SqliteManager sq;
	sq.Open("test.db");

	string Create_table_sql = "create table tb_doc(id int primary key autoincrement,doc_path text,doc_name text)";
	sq.ExecuteSql(Create_table_sql);

	string Insert_sql;
	Insert_sql = "insert into tb_doc values(NULL,'giturtle','CSDN')";
	sq.ExecuteSql(Insert_sql);
	Insert_sql = "insert into tb_doc values(NULL,'apple','red')";
	sq.ExecuteSql(Insert_sql);
	Insert_sql = "insert into tb_doc values(NULL,'banana','yellow')";
	sq.ExecuteSql(Insert_sql);

	string Query_sql = "select * from tb_doc where doc_path = 'apple\'";
	int row, col;
	char** ppRet;
	sq.GetTable(Query_sql, row, col, ppRet);

	//for (int i = 1; i < row; ++i) {		//按一维数组放置,第一行数据为表字段的名称，所以舍弃
	//	for (int j = 0; j < col; ++j) {
	//		cout << ppRet[i*col + j] << " ";
	//	}
	//	cout << endl;
	//}
	sqlite3_free_table(ppRet);		//使用完毕释放二维表sqlite3_malloc申请的内存，否则内存泄漏

	AutoGetTable agt(sq, Query_sql, row, col, ppRet);		//因为数据库对象中有打开数据库的连接

	for (int i = 1; i <= row; ++i) {
		for (int j = 0; j < col; ++j) {
			cout << ppRet[i*col + j] << " ";	//底层也只是一个一维数组，因为二维数组的动态开辟很消耗资源
		}
		cout << endl;
	}
	//此时通过RAII思想，已经释放了sqlite3_malloc申请的内存
}
void TestScanManager() {
	ScanManager::CreateInstance()->StartScan();
}

void TestSearch() {
	
	DataManager::GetInstance();

	vector<std::pair<string, string>> docinfos;
	string key;
	cout << "======================================" << endl;
	cout << "请输入要搜索的关键字:";
	while (std::cin >> key) {
		docinfos.clear();
		DataManager::GetInstance()->Search(key, docinfos);
		printf("%-50s %-50s\n", "名称", "路径");
		for (const auto& e : docinfos) {
			//cout << e.first << "  " << e.second << endl;
			//printf("%-50s %-50s\n", e.first.c_str(), e.second.c_str());

			//first 这一项str需要高亮
			string prefix, highlight, suffix;
			const string& name = e.first;
			const string& path = e.second;

			DataManager::GetInstance()->SpliteHighlight(e.first, key, prefix, highlight, suffix);
			cout << prefix;
			ColourPrintf(highlight.c_str());
			cout << suffix;

			//对齐，补空格
			for (size_t i = name.size(); i <= 50; ++i) {
				cout << " ";
			}

			printf("%-50s\n", path.c_str());	//这一项可以正常输出
		}
		cout << "======================================" << endl;
		cout << "请输入要搜索的关键字:";
	}
}

void TestPinyin() {
	string allspell = ChineseConvertPinYinAllSpell("拼音 IO多路转接");
	string initals = ChineseConvertPinYinInitials("拼音 IO多路转接");
	cout << allspell << endl;
	cout << initals << endl;
}

void TestHighLight() {
	//ColourPrintf("miaomiaomiao~\n");

	//1. key是什么，高亮key
	{
		string key = "语言";
		string str = "php是世界上最好的语言哈哈哈哈";
		string prefix, suffix;

		size_t pos = str.find(key);
		if (pos != string::npos) {		//表示匹配上了
			prefix = str.substr(0, pos);	//substr不到pos，是pos前一位
			suffix = str.substr(pos + key.size(), string::npos);
		}
		cout << prefix;
		ColourPrintf(key.c_str());
		cout << suffix << endl;
	}

	//2. key是拼音，高亮对应的汉字
	{
		string key = "yuyan";
		string str = "php是世界上最好的语言哈哈哈哈";
		string prefix, suffix;

		string key_pinyin = ChineseConvertPinYinAllSpell(key);
		string str_pinyin = ChineseConvertPinYinAllSpell(str);
		size_t pos = str_pinyin.find(key_pinyin);
		if (pos == string::npos)
			cout << "拼音不匹配" << endl;
		else {
			size_t key_start = pos;
			size_t key_end = pos + key_pinyin.size();
			size_t str_i = 0, py_i = 0;
			while (py_i < key_start) {
				if (str[str_i] >= 0 && str[str_i] <= 127) {	//ASCII字符
					++str_i;
					++py_i;
				}
				else {
					char chinese[3];		//如果最后不设置\0，结尾会翻译为烫烫烫
					chinese[2] = '\0';
					chinese[0] = str[str_i];
					chinese[1] = str[str_i + 1];
					str_i += 2;

					string py_str = ChineseConvertPinYinAllSpell(chinese);
					py_i += py_str.size();
				}
			}

			prefix = str.substr(0, str_i);

			size_t str_j = str_i, py_j = py_i;
			while (py_j < key_end) {
				if (str[str_j] >= 0 && str[str_j] <= 127) {	
					++str_j;
					++py_j;
				}
				else {
					char chinese[3];
					chinese[2] = '\0';
					chinese[0] = str[str_j];
					chinese[1] = str[str_j + 1];

					str_j += 2;

					string py_str = ChineseConvertPinYinAllSpell(chinese);
					py_j += py_str.size();
				}
			}
			key = str.substr(str_i, str_j - str_i);
			suffix = str.substr(str_j, string::npos);

			cout << prefix;
			ColourPrintf(key.c_str());
			cout << suffix << endl;
		}
	}

	////3. key是拼音首字母，高亮对应的汉字
	//{
	//	string key = "yy";
	//	string str = "php是世界上最好的语言哈哈哈哈";
	//	size_t pos = str.find(key);
	//	string prefix, suffix;
	//	prefix = str.substr(0, pos);
	//	suffix = str.substr(pos + key.size(), string::npos);
	//	cout << prefix;
	//	ColourPrintf(key.c_str());
	//	cout << suffix << endl;
	//}
}
int main() {
	TestSqlite();
	//TestScanManager();
	//TestSearch();	//界面
	//TestPinyin();
	//TestHighLight();
	system("pause");
	return 0;
}
