#include <cstring>
#include <iostream>

class String {
private:
  size_t sz_;
  size_t cap_;
  char* array_;

  explicit String(size_t count): sz_(count), cap_(count + 1), array_(new char[cap_]) {
    array_[sz_] = '\0';
  }
  void Swap(String& other) {
    std::swap(sz_, other.sz_);
    std::swap(cap_, other.cap_);
    std::swap(array_, other.array_);
  }
  void Reallocate(size_t new_cap_) {
    if (new_cap_ == 0) {
      std::cerr << "Error: zero cap_ reallocation\n";
      return;
    }
    if (sz_ > new_cap_) {
      std::cerr << "Error: sz_ is greater than cap_ while reallocation";
      return;
    }
    String new_str(new_cap_ - 1);
    new_str.sz_ = sz_;
    memcpy(new_str.array_, array_, sz_ + 1);
    Swap(new_str);
  }

public:
  String(const char* carr): String(std::strlen(carr)) {
    memcpy(array_, carr, sz_);
  }
  String(size_t count, char symbol): String(count) {
    memset(array_, symbol, count);
  }
  String(): sz_(0), cap_(1), array_(new char[cap_]) {
    *array_ = '\0';
  }
  String(const String& other): sz_(other.sz_), cap_(other.cap_),
                               array_(new char[cap_]) {
    memcpy(array_, other.array_, sz_ + 1);
  }
  String(char symbol): sz_(1), cap_(2), array_(new char[cap_]) {
    array_[0] = symbol;
    array_[1] = '\0';
  }
  ~String() {
    delete[] array_;
  }
  String& operator=(const String& other) {
    if (array_ == other.array_) {
      return *this;
    }
    String copy = other;
    Swap(copy);
    return *this;
  }
  char& operator[](size_t index) {
    return array_[index];
  }
  const char& operator[](size_t index) const {
    return array_[index];
  }
  String& operator+=(const char symbol) {
    if (sz_ + 2 > cap_) {
      Reallocate(cap_ * 2);
    }
    array_[sz_] = symbol;
    array_[sz_ + 1] = '\0';
    ++sz_;
    return *this;
  }
  String& operator+=(const String& other) {
    if (other.sz_ == 0) {
      return *this;
    }
    size_t new_sz_ = sz_ + other.sz_ + 1;
    if (new_sz_ > cap_) {
      Reallocate(new_sz_);
    }
    memcpy(array_ + sz_ + 1, other.array_ + 1, other.sz_);
    array_[sz_] = other.array_[0];
    sz_ = new_sz_ - 1;
    return *this;
  }

  size_t size() const {
    return sz_;
  }
  size_t capacity() const {
    return cap_ - 1;
  }
  size_t length() const {
    return sz_;
  }
  bool empty() const {
    return sz_ == 0;
  }
  void clear() {
    sz_ = 0;
    array_[sz_] = '\0';
  }
  char* data() {
    return array_;
  }
  const char* data() const {
    return array_;
  }
  char& front() {
    if (sz_ == 0) {
      std::cerr << "Error: string is empty\n";
    }
    char& result = array_[0];
    return result;
  }
  const char& front() const {
    if (sz_ == 0) {
      std::cerr << "Error: string is empty\n";
    }
    const char& result = array_[0];
    return result;
  }
  char& back() {
    if (sz_ == 0) {
      std::cerr << "Error: string is empty\n";
    }
    return array_[sz_ - 1];
  }
  const char& back() const {
    if (sz_ == 0) {
      std::cerr << "Error: string is empty\n";
    }
    return array_[sz_ - 1];
  }
  void push_front(const char symbol) {
    if (sz_ + 2 > cap_) {
      Reallocate(cap_ * 2);
    }
    memmove(array_ + 1, array_, sz_ + 1);
    array_[0] = symbol;
    ++sz_;
  }
  void push_back(const char symbol) {
    *this += symbol;
  }
  void pop_back() {
    if (empty()) {
      std::cerr << "Error: pop_back from empty string\n";
      return;
    }
    --sz_;
    array_[sz_] = '\0';
  }
  void shrink_to_fit() {
    if (cap_ > sz_ + 1) {
      Reallocate(sz_ + 1);
    }
  }
  String substr(size_t index, size_t count) const {
    if (index + count > sz_) {
      std::cerr << "Error: out of range substr";
    }
    String result(count);
    memcpy(result.array_, array_ + index, count);
    return result;
  }
  size_t find(const String& substr) const {
    for (size_t i = 0; i <= size() - substr.size(); ++i) {
      if (memcmp(array_ + i, substr.array_, substr.sz_) == 0) {
        return i;
      }
    }
    return size();
  }
  size_t rfind(const String& substr) const {
    for (size_t i = size() - substr.size() + 1; i-- > 0;) {
      if (memcmp(array_ + i, substr.array_, substr.sz_) == 0) {
        return i;
      }
    }
    return size();
  }
};
bool operator==(const String& that, const String& other) {
  return that.size() != other.size() ? false : memcmp(that.data(), other.data(), that.size()) == 0;
}
bool operator!=(const String& that, const String& other) {
  return !(that == other);
}
bool operator<(const String& that, const String& other) {
  int cmp_res = memcmp(that.data(), other.data(), std::min(that.size(), other.size()));
  return cmp_res == 0 ? that.size() < other.size() : cmp_res < 0;
}
bool operator>=(const String& that, const String& other) {
  return !(that < other);
}
bool operator>(const String& that, const String& other) {
  return other < that;
}
bool operator<=(const String& that, const String& other) {
  return !(that > other);
}
String operator+(const String& first_str, const String& second_str) {
  String new_str = first_str;
  new_str += second_str;
  return new_str;
}

std::ostream& operator<<(std::ostream& out, const String& str) {
  for(size_t i = 0; i < str.size(); ++i) {
    out << str[i];
  }
  return out;
}

std::istream& operator>>(std::istream& in, String& str) {
  char input_c;
  str.clear();
  str.shrink_to_fit();
  while (in.get(input_c)) {
    if (std::isspace(input_c)) {
      break;
    }
    str += input_c;
  }
  return in;
}
