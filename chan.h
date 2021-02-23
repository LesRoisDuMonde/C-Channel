#pragma once
#include <condition_variable>
#include <list>
#include <mutex>

template<typename T>
class ChanQueue {
public:
	explicit ChanQueue(size_t cap) : capacity_(cap == 0 ? 1 : cap), 
		enable_overflow_(cap == 0), closed_(false), pop_count_(0){
	}

	inline bool Empty() const {
		return data_.empty();
	}

	inline size_t FreeCount() const {
		return capacity_ - data_.size();
	}

	inline bool IsOverFlow() const {
		return enable_overflow_ && data_.size() >= capacity_;
	}

	inline  bool IsClosed() const {
		std::unique_lock<std::mutex> lock(this->mutex_);
		return this->closed_;
	}

	inline void Close() {
		std::unique_lock<std::mutex> lock(this->mutex_);
		this->closed_ = true;
		if (this->IsOverFlow()) {
			this->data_.pop_back();
		}
		this->cv_.notify_all();
	}

	template <typename R>
	inline bool pop(R&& data){
		std::unique_lock<std::mutex> lock(this->mutex_);
		this->cv_.wait(lock, [&]() {return !Empty() || closed_;});
		if(this->Empty()){
			return false;
		}

		data = this->data_.front();
		this->data_.pop_front();
		this->pop_count_++;

		if (this->FreeCount() == 1 ) {
			this->cv_.notify_all();
		}

		return true;
	}

	template<typename R>
	inline bool push(R&& data) {
		std::unique_lock<std::mutex> lock(this->mutex_);
		this->cv_.wait(lock, [&]() {return FreeCount() > 0 || this->closed_;});
		if (this->closed_) {
			return false;
		}

		this->data_.emplace_back(data);

		if (this->data_.size() == 1) {
			this->cv_.notify_all();
		}

		if (this->IsOverFlow()) {
			const size_t old = this->pop_count_;
			this->cv_.wait(lock, [&]() {return old != this->pop_count_ || this->closed_;});
		}

		return !this->closed_;
	}
private:
	mutable std::mutex mutex_;
	std::condition_variable cv_;
	std::list<T> data_;
	const size_t capacity_;
	const bool enable_overflow_;
	bool closed_;
	size_t pop_count_;
};

template<typename T>
class Chan {
public:
	explicit Chan(size_t cap = 0) {
		queue_ = std::make_shared<ChanQueue<T>>(cap);
	}

	Chan(const Chan &) = default;
	Chan& operator= (const Chan&) = default;

	Chan(Chan&&) = default;
	Chan& operator=(Chan&&) = default;

	template<typename R>
	void operator<<(R && data){
		queue_->push(data);
	}

	template<typename R>
	bool operator>>(R && data) {
		return queue_->pop(data);
	}

	inline void Close() {
		queue_->Close();
	}

	inline  bool IsClosed() const {
		return queue_->IsClosed();
	}

private:
	std::shared_ptr<ChanQueue<T>> queue_;
};
