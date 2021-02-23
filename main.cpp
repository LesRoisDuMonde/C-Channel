#include <iostream>
#include <thread>
#include "chan.h"

using namespace std::chrono;

void consume(Chan<int>& ch, int thId) {
	int n = 0;
	while (ch >> std::move(n)) {
		std::cout << thId << " : " << n << std::endl;
		std::this_thread::sleep_for(milliseconds(1000));
	}
}

int main() {
	Chan<int> chInt(3);
		
	// 消费者
	std::thread consumers[5];
	for (int i = 0; i < 5; i++) {
		consumers[i] = std::thread(consume, chInt, i + 1);
	}

	// 生产数据 
	for (int i = 0; i < 16; i++) {
		chInt << std::move(i);
	}
	chInt.Close();  // 数据生产完毕

	for (std::thread & thr : consumers) {
		thr.join();
	}

	return 0;
}
