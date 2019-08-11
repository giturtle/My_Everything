

#include "Common.h"
#include "ScanManager.h"

void ScanManager::Scan(const string& path) {
	
	//对比 文件系统和数据库
	vector<string> localdirs;
	vector<string> localfiles;
	DirectoryList(path, localdirs, localfiles);	//区分的原因：文档需要递归查找

	std::set<string> localset;
	localset.insert(localfiles.begin(), localfiles.end());
	localset.insert(localdirs.begin(), localdirs.end());

	//数据库的
	std::set<string> dbset;
	DataManager::GetInstance()->GetDocs(path, dbset);
	
	auto localit = localset.begin();
	auto dbit = dbset.begin();
	while (localit != localset.end() && dbit != dbset.end()) {
		if (*localit < *dbit) {			//新增数据(本地有，数据库没有)
			DataManager::GetInstance()->InsertDoc(path, *localit);
			++localit;
		}
		else if (*localit > *dbit) {	//删除数据(本地没有，数据库有)
			DataManager::GetInstance()->DeleteDoc(path, *dbit);
			++dbit;
		}
		else {		//不变的数据
			++localit;
			++dbit;
		}
	}
	while (localit != localset.end()) {
		//新增数据
		DataManager::GetInstance()->InsertDoc(path, *localit);
		++localit;
	}
	while (dbit != dbset.end()) {
		//删除数据
		DataManager::GetInstance()->DeleteDoc(path, *dbit);
		++dbit;
	}
	//递归扫描对比子目录数据
	for (const auto& subdirs : localdirs) {		//以本地数据为主
		string subpath = path;
		subpath += '\\';
		subpath += subdirs;

		Scan(subpath);
	}
}
