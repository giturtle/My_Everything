#pragma once

#include "Common.h"
#include "DataManager.h"

//ScanManager设计为单例模式：整个程序只有一个扫描模块
class ScanManager {
public:
	void Scan(const string& path);

	void StartScan() {
		while (1) {
			Scan("D:\\Common");
			std::this_thread::sleep_for(std::chrono::seconds(5));	//每过3秒钟扫一次
		}
	}

	static ScanManager* CreateInstance() {
		static ScanManager scanmgr;
		static std::thread thd(&StartScan, &scanmgr);		//scanmgr就传给了this
		thd.detach();	//分离线程，互相独立

		return &scanmgr;
	}
private:
	//构造函数私有化
	ScanManager() {
		//_datamgr.Init();
	}

	ScanManager(const ScanManager&);
	//DataManager _datamgr;
};

