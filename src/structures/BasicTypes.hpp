#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>


class Text {
    char* p_ = nullptr;
    size_t n_ = 0;
public:
    static constexpr size_t npos = size_t(-1);
    Text() = default;
    Text(const char* s) : n_(strlen(s)) { p_ = new char[n_+1]; memcpy(p_,s,n_+1); }
    Text(size_t n, char c) : p_(new char[n+1]), n_(n) { memset(p_,c,n); p_[n]=0; }
   
    template<class S, class = decltype(static_cast<const S*>(nullptr)->c_str())>
    Text(const S& s) : Text(s.c_str()) {}
    template<class S, class = decltype(S(static_cast<const char*>(nullptr)))>
    operator S() const { return S(c_str()); }
    Text(const Text& s) : Text(s.c_str()) {}
    Text(Text&& s) noexcept : p_(s.p_), n_(s.n_) { s.p_=nullptr; s.n_=0; }
    ~Text() { delete[] p_; }
    Text& operator=(Text s) noexcept { char* p=p_; p_=s.p_; s.p_=p; size_t n=n_; n_=s.n_; s.n_=n; return *this; }
    const char* c_str() const { return p_ ? p_ : ""; }
    size_t size() const { return n_; }
    size_t capacity() const { return n_; }
    bool empty() const { return n_==0; }
    char& operator[](size_t i) { return p_[i]; }
    const char& operator[](size_t i) const { return p_[i]; }
    char* begin() { return p_; }
    char* end() { return n_ ? p_+n_ : p_; }
    const char* begin() const { return c_str(); }
    const char* end() const { return c_str()+n_; }
    Text substr(size_t start, size_t count) const { if(start>n_) start=n_; if(count>n_-start) count=n_-start; Text r(count,' '); if(count) memcpy(r.p_,c_str()+start,count); return r; }
    size_t find(const Text& s) const { const char* b=c_str(); const char* r=strstr(b,s.c_str()); return r ? size_t(r-b) : npos; }
    friend bool operator==(const Text& a,const Text& b) { return strcmp(a.c_str(),b.c_str())==0; }
    friend bool operator!=(const Text& a,const Text& b) { return !(a==b); }
    friend bool operator<(const Text& a,const Text& b) { return strcmp(a.c_str(),b.c_str())<0; }
    friend Text operator+(const Text& a,const Text& b) { Text r(a.n_+b.n_,' '); memcpy(r.p_,a.c_str(),a.n_); memcpy(r.p_+a.n_,b.c_str(),b.n_); return r; }
};
template<class T> Text numberText(T n) { char b[32]; snprintf(b,sizeof b,"%lld",static_cast<long long>(n)); return Text(b); }
class ProjectError { Text message_; public: explicit ProjectError(const Text& s):message_(s){} const char* what() const {return message_.c_str();} };

class Output { FILE* f_; public: explicit Output(FILE* f):f_(f){} Output& operator<<(const char* s){fputs(s,f_);return *this;} Output& operator<<(const Text& s){return *this<<s.c_str();} Output& operator<<(char c){fputc(c,f_);return *this;} Output& operator<<(double d){fprintf(f_,"%g",d);return *this;} template<class T> Output& operator<<(T n){fprintf(f_,"%lld",static_cast<long long>(n));return *this;} };
inline Output consoleOut(stdout), consoleErr(stderr);
inline double secondsNow(){ return double(clock())/CLOCKS_PER_SEC; }
class RandomGenerator { uint32_t state_; public: explicit RandomGenerator(uint32_t seed):state_(seed?seed:1){} uint32_t next(){state_^=state_<<13;state_^=state_>>17;state_^=state_<<5;return state_;} };
template<class T> class RandomRange { T lo_,hi_; public: RandomRange(T lo,T hi):lo_(lo),hi_(hi){} T operator()(RandomGenerator& r){return lo_+T(r.next()%(uint64_t(hi_)-lo_+1));} };


template<class T> class SearchArray {
    T* data_ = nullptr;
    size_t size_ = 0, capacity_ = 0;
    void exchange(SearchArray& other) noexcept {
        T* p = data_; data_ = other.data_; other.data_ = p;
        size_t n = size_; size_ = other.size_; other.size_ = n;
        n = capacity_; capacity_ = other.capacity_; other.capacity_ = n;
    }
public:
    SearchArray() = default;
    template<class... Rest> SearchArray(const T& first, const Rest&... rest) : SearchArray() {
        reserve(1 + sizeof...(rest)); push_back(first); (push_back(rest), ...);
    }
    SearchArray(const SearchArray& other) : SearchArray() {
        reserve(other.size_); for (const T& value : other) push_back(value);
    }
    SearchArray(SearchArray&& other) noexcept { exchange(other); }
    SearchArray& operator=(SearchArray other) noexcept { exchange(other); return *this; }
    ~SearchArray() { delete[] data_; }
    size_t size() const { return size_; }
    size_t capacity() const { return capacity_; }
    bool empty() const { return size_ == 0; }
    T& operator[](size_t i) { return data_[i]; }
    const T& operator[](size_t i) const { return data_[i]; }
    T* begin() { return data_; }
    const T* begin() const { return data_; }
    T* end() { return size_ ? data_ + size_ : data_; }
    const T* end() const { return size_ ? data_ + size_ : data_; }
    void reserve(size_t n) {
        if (n <= capacity_) return;
        if (n > size_t(-1)/sizeof(T)) throw ProjectError("array capacity exceeded");
        T* replacement = new T[n];
        try { for (size_t i=0; i<size_; ++i) replacement[i] = data_[i]; }
        catch (...) { delete[] replacement; throw; }
        delete[] data_; data_ = replacement; capacity_ = n;
    }
    void push_back(const T& value) {
        if (size_ == capacity_) {
            T copy(value); // safe even if value refers to this array
            if (capacity_ > size_t(-1)/2) throw ProjectError("array capacity exceeded");
            reserve(capacity_ ? capacity_*2 : 1);
            data_[size_] = copy;
        } else data_[size_] = value;
        ++size_;
    }
    
    template<class Array> Array toArray() const {
        Array result; result.reserve(size_);
        for (const T& item : *this) result.push_back(item);
        return result;
    }
};
