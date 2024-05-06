#include <algorithm>
#include <iostream>
#include <cstring>
#include <string>
#include <vector>

enum class Sign {
  Positive,
  Negative,
  Zero
};

Sign operator*(Sign a, Sign b) {
  if (a == Sign::Zero || b == Sign::Zero) {
    return Sign::Zero;
  }

  if (a == b) {
    return Sign::Positive;
  }

  return Sign::Negative;
}

class Rational;

class BigInteger;

bool operator<(const BigInteger& that, const BigInteger& other);

bool operator==(const BigInteger& that, const BigInteger& other);

BigInteger operator-(int, const BigInteger&);

class BigInteger {
private:
  Sign sign_ = Sign::Zero;
  size_t digit_cnt_;
  std::vector<long long> digits_;

  void swap(BigInteger& other);

  void updateOneDigitSimple(size_t index);

  void updateDigitsSimple(size_t begin);

  void updateDigits(size_t begin);

  void updateDigitsBack(size_t begin);

  void updateDigitsDeleteEmpty();

  long long getHighDigit() const;

  static long long ratioBinarySearch(const BigInteger& first, const BigInteger& second);

  static void addZerosToSymbol(std::string& symbol);

  static BigInteger divisionPositive(BigInteger& dividend, const BigInteger& divisor);

  static Sign signProduct(const BigInteger& first, const BigInteger& second);

  static BigInteger gcd(const BigInteger& first, const BigInteger& second);

  size_t makeGreaterThan(const BigInteger& other);

  std::string toFullString() const;

  void inverse();

  bool isZero() const;

  bool isPositive() const;

  bool isNegative() const;

  friend class Rational;
  friend bool operator<(const BigInteger&, const BigInteger&);
  friend bool operator==(const BigInteger&, const BigInteger&);
  friend BigInteger operator-(int, const BigInteger&);

public:
  static const long long base = 1e9;
  static const size_t base_power = 9;

  ~BigInteger() = default;

  BigInteger() = default;

  BigInteger& operator=(const BigInteger& source) = default;

  BigInteger(const BigInteger& source) = default;

  BigInteger(int source);

  size_t getDigitCount() const;

  const std::vector<long long>& getDigits() const;

  explicit BigInteger(long long source);

  explicit BigInteger(const std::string& source);

  explicit operator bool() const;

  std::string toString() const;

  BigInteger operator-() const;

  BigInteger& operator+=(const BigInteger& other);

  BigInteger& operator+=(long long other);

  BigInteger operator++(int);

  BigInteger& operator++();

  BigInteger operator--(int);

  BigInteger& operator--();

  BigInteger& operator-=(const BigInteger& other);

  BigInteger& operator*=(const BigInteger& other);

  BigInteger& operator*=(long long other);

  BigInteger& operator<<(size_t value);

  BigInteger& operator/=(const BigInteger& other);

  BigInteger& operator%=(const BigInteger& other);
};


bool operator<(const BigInteger& that, const BigInteger& other);

bool operator==(const BigInteger& that, const BigInteger& other);

bool operator>(const BigInteger& that, const BigInteger& other);

bool operator>=(const BigInteger& that, const BigInteger& other);

bool operator<=(const BigInteger& that, const BigInteger& other);

void BigInteger::swap(BigInteger& other) {
  std::swap(sign_, other.sign_);
  std::swap(digit_cnt_, other.digit_cnt_);
  std::swap(digits_, other.digits_);
}

void BigInteger::updateOneDigitSimple(size_t index) {
  if (digits_[index] >= base) {

    if (index + 1 == digit_cnt_) {
      digits_.push_back(0);
      ++digit_cnt_;
    }

    ++digits_[index + 1];
    digits_[index] -= base;
  }
}

void BigInteger::updateDigitsSimple(size_t begin) {
  size_t index = begin;

  while (index < digit_cnt_ - 1 && digits_[index] >= base) {
    ++digits_[index + 1];
    digits_[index] -= base;
    ++index;
  }

  if (digits_[index] >= base) {
    digits_[index] -= base;
    digits_.push_back(1ll);
  }
}

void BigInteger::updateDigits(size_t begin) {
  for (size_t index = begin; index < digit_cnt_ - 1; ++index) {

    if (digits_[index] >= base) {
      long long temp = digits_[index] / base;
      digits_[index + 1] += temp;
      digits_[index] -= temp * base;
    }

  }

  if (getHighDigit() >= base) {
    long long temp = getHighDigit() / base;
    digits_.push_back(temp);
    digits_[digit_cnt_ - 1] -= temp * base;
    ++digit_cnt_;
  }
}

void BigInteger::updateDigitsBack(size_t begin) {
  if (digits_[begin] < 0) {

    size_t index = begin + 1;

    while (digits_[index] == 0) {
     ++index;
    }

    --digits_[index];

    if (digits_[index] == 0 && index + 1 == digit_cnt_) {
      digits_.pop_back();
      --digit_cnt_;
    }

    digits_[--index] += base;

    while (index > begin) {
      --digits_[index--];
      digits_[index] += base;
    }
  }
}
void BigInteger::updateDigitsDeleteEmpty() {
  if (digit_cnt_ <= 0) {
    return;
  }

  size_t index = digit_cnt_ - 1;

  while (digits_[index] == 0) {
    digits_.pop_back();
    --digit_cnt_;

    if (index == 0) {
      sign_ = Sign::Negative;
      break;
    }

    --index;
  }
}

long long BigInteger::getHighDigit() const {
  if (isZero()) {
    return 0;
  }

  return digits_[digit_cnt_ - 1];
}

long long BigInteger::ratioBinarySearch(const BigInteger& first, const BigInteger& second) {
  if (second.sign_ == Sign::Negative) {
    return -1;
  }

  if (first.sign_ * second.sign_ == Sign::Negative || first.sign_ == Sign::Negative) {
    return -1;
  }

  if (second > first) {
    return 0;
  }

  if (second == first) {
    return 1;
  }

  long long ratio_max = base + 1;
  long long ratio_min = 1;
  long long ratio_mid;
  BigInteger temp;

  while (ratio_min < ratio_max - 1) {
    ratio_mid = (ratio_max + ratio_min) / 2;
    temp = second;
    temp *= ratio_mid;

    if (temp <= first) {

      ratio_min = ratio_mid;

    } else {

      ratio_max = ratio_mid;

    }
  }
  return ratio_min;
}

void BigInteger::addZerosToSymbol(std::string& symbol) {
  if (symbol.size() < base_power) {
    symbol = std::string(base_power - symbol.size(), '0') + symbol;
  }
}

BigInteger BigInteger::gcd(const BigInteger& first, const BigInteger& second) {
  BigInteger first_temp(first);
  BigInteger second_temp(second);
  BigInteger temp;

  if (!first_temp.isPositive() || !second_temp.isPositive()) {
    std::cerr << "GCD wrong sign_s" << std::endl;
    return temp;
  }

  while (second_temp.isPositive()) {
    temp = second_temp;
    second_temp = first_temp %= second_temp;
    second_temp.updateDigitsDeleteEmpty();
    first_temp = temp;
  }

  return first_temp;
}

BigInteger BigInteger::divisionPositive(BigInteger& dividend, const BigInteger& divisor) {
  BigInteger divisor_temp(divisor);
  std::vector<long long> reverse_answer;

  BigInteger dividend_temp;
  dividend_temp.sign_ = Sign::Positive;

  for (size_t index = dividend.digit_cnt_ - divisor.digit_cnt_; index < dividend.digit_cnt_; ++index) {
    dividend_temp.digits_.push_back(dividend.digits_[index]);
  }

  dividend_temp.digit_cnt_ = divisor.digit_cnt_;
  long long temp_ans = ratioBinarySearch(dividend_temp, divisor_temp);
  reverse_answer.push_back(temp_ans);

  divisor_temp *= temp_ans;
  dividend_temp -= divisor_temp;

  for (size_t index = dividend.digit_cnt_ - divisor.digit_cnt_; index > 0; --index) {
    divisor_temp = divisor;
    dividend_temp << static_cast<size_t>(1);
    dividend_temp += dividend.digits_[index - 1];

    temp_ans = ratioBinarySearch(dividend_temp, divisor_temp);

    reverse_answer.push_back(temp_ans);
    divisor_temp *= temp_ans;
    dividend_temp -= divisor_temp;
    dividend_temp.updateDigitsDeleteEmpty();
  }

  std::reverse(reverse_answer.begin(), reverse_answer.end());
  dividend.digits_ = reverse_answer;
  dividend.digit_cnt_ = reverse_answer.size();
  dividend.updateDigitsDeleteEmpty();

  return dividend_temp;
}

Sign BigInteger::signProduct(const BigInteger& first, const BigInteger& second) {
  return first.sign_ * second.sign_;
}

BigInteger::BigInteger(int source): sign_(Sign::Zero), digit_cnt_(0) {
  if (source > 0) {

    sign_ = Sign::Positive;

  } else if (source < 0) {

    sign_ = Sign::Negative;

  } else {

    return;

  }

  digit_cnt_ = abs(source) >= base ? 2 : 1;
  int temp = abs(source);

  long long digit_first = temp % base;
  digits_.push_back(digit_first);
  long long digit_second = (temp - digit_first) / base;

  if (digit_second) {
    digits_.push_back(digit_second);
  }
}

void BigInteger::inverse() {
  if (sign_ == Sign::Positive) {

    sign_ = Sign::Negative;

  } else if (sign_ == Sign::Negative) {

    sign_ = Sign::Positive;

  }
}

size_t BigInteger::getDigitCount() const {
  return digit_cnt_;
}

const std::vector<long long>& BigInteger::getDigits() const {
  return digits_;
}

bool BigInteger::isZero() const {
  return sign_ == Sign::Zero;
}

bool BigInteger::isPositive() const {
  return sign_ == Sign::Positive;
}

bool BigInteger::isNegative() const {
  return sign_ == Sign::Negative;
}

BigInteger::BigInteger(long long source): sign_(Sign::Zero), digit_cnt_(0) {
  if (source > 0) {

    sign_ = Sign::Positive;

  } else if (source < 0) {

    sign_ = Sign::Negative;

  } else {

    return;
  }

  long long temp_digit;
  long long copy = source;

  while (copy > 0) {
    temp_digit = copy % base;
    digits_.push_back(temp_digit);
    copy -= temp_digit;
    copy /= base;
  }

  digit_cnt_ = digits_.size();
}

BigInteger::BigInteger(const std::string& source): sign_(Sign::Zero), digit_cnt_(0) {
  if (source.size() != 0) {

    size_t lower_bound = 0;

    if (source[0] == '-') {

      sign_ = Sign::Negative;
      lower_bound = 1;

    } else {

      sign_ = Sign::Positive;
    }

    if (source[lower_bound] == '0') {
      *this = BigInteger();
    }

    size_t new_ind = source.size();

    for (; new_ind >= lower_bound + base_power; new_ind -= base_power) {
      std::string substr = source.substr(new_ind - base_power, base_power);
      digits_.push_back(std::stoll(substr));
      ++digit_cnt_;
    }

    if (new_ind != lower_bound) {
      std::string substr = source.substr(lower_bound, new_ind - lower_bound);
      digits_.push_back(std::stoll(substr));
      ++digit_cnt_;
    }
  }
}

size_t BigInteger::makeGreaterThan(const BigInteger& other) {
  if (*this >= other) {

    return static_cast<size_t>(0);
  }

  size_t digit_diff = other.digit_cnt_ - digit_cnt_;
  *this << digit_diff;

  if (*this < other) {
    *this << static_cast<size_t>(1);
    ++digit_diff;
  }

  return digit_diff;
}

BigInteger::operator bool() const {
  return sign_ != Sign::Zero;
}

std::string BigInteger::toString() const {
  if (sign_ == Sign::Zero) {

    return std::string(1, '0');
  }

  std::string result;
  std::string str_temp;

  if (sign_ == Sign::Negative) {
    result += '-';
  }

  result += std::to_string(digits_[digit_cnt_ - 1]);

  for (size_t i = digit_cnt_ - 1; i > 0; --i) {
    str_temp = std::to_string(digits_[i - 1]);
    addZerosToSymbol(str_temp);
    result += str_temp;
  }

  return result;
}

std::string BigInteger::toFullString() const {
  std::string result;

  if (sign_ == Sign::Zero) {
    return result;
  }

  if (sign_ == Sign::Negative) {
    result += '-';
  }

  for (size_t index = digit_cnt_; index > 0; --index) {
    std::string temp(std::to_string(digits_[index - 1]));
    addZerosToSymbol(temp);
    result += temp;
  }

  return result;
}

BigInteger BigInteger::operator-() const {
  BigInteger result(*this);
  result.inverse();
  return result;
}

BigInteger& BigInteger::operator+=(const BigInteger& other) {
  if (isZero()) {

    *this = other;
    return *this;
  }

  if (other.isZero()) {
    return *this;
  }

  if (sign_ * other.sign_ == Sign::Positive) {

    if (other.digit_cnt_ > digit_cnt_) {
       digit_cnt_ = other.digit_cnt_;
       digits_.resize(digit_cnt_);
    }

    for (size_t i = 0; i < std::min(other.digit_cnt_, digit_cnt_); ++i) {
      digits_[i] += other.digits_[i];
      updateOneDigitSimple(i);
    }

  } else {

    inverse();

    if ((*this) == other) {
      *this = BigInteger();
      return *this;
    }

    if ((sign_ == Sign::Negative) ^ ((*this) < other)) {

      BigInteger temp(other);

      for (size_t i = 0; i < digit_cnt_; ++i) {
        temp.digits_[i] -= digits_[i];
        temp.updateDigitsBack(i);
      }

      swap(temp);

    } else {

      for (size_t i = 0; i < other.digit_cnt_; ++i) {
        digits_[i] -= other.digits_[i];
        updateDigitsBack(i);
      }

      inverse();
    }
  }

  return *this;
}

BigInteger& BigInteger::operator+=(long long other) {
  if (other == 0){
    return *this;
  }

  if (isZero()) {
    *this = BigInteger();
    sign_ = other > 0 ? Sign::Positive : Sign::Negative;
    digits_.push_back(other);
    digit_cnt_ = 1;
    return *this;
  }

  Sign other_sign = other > 0 ? Sign::Positive : Sign::Negative;

  if (sign_ * other_sign == Sign::Positive) {

    digits_[0] += other;
    updateDigitsSimple(0);

  } else {

    if (digit_cnt_ == 1 && digits_[0] == other) {
      *this = BigInteger();
      return *this;
    }

    digits_[0] -= other;

    if (digits_[0] < 0) {

      if (digit_cnt_ == 1) {

        inverse();
        digits_[0] *= -1;

      } else {

        updateDigitsBack(0);
      }
    }
  }

  return *this;
}

BigInteger BigInteger::operator++(int) {
  BigInteger copy(*this);
  *this += 1ll;
  return copy;
}

BigInteger& BigInteger::operator++() {
  *this += 1ll;
  return *this;
}

BigInteger BigInteger::operator--(int) {
  BigInteger copy(*this);
  *this += -1ll;
  return copy;
}

BigInteger& BigInteger::operator--() {
  *this += -1ll;
  return *this;
}

BigInteger& BigInteger::operator-=(const BigInteger& other) {
  if (this == &other) {
    *this = BigInteger();
    return *this;
  }

  inverse();
  *this += other;
  inverse();

  return *this;
}

BigInteger& BigInteger::operator*=(const BigInteger& other) {
  BigInteger result;

  if (sign_ * other.sign_ == Sign::Zero) {
    swap(result);
    return *this;
  }

  result.sign_ = sign_ * other.sign_;
  result.digit_cnt_ = digit_cnt_ + other.digit_cnt_ - 1;
  result.digits_ = std::vector<long long>(result.digit_cnt_, 0);

  for (size_t index_oth = 0; index_oth < other.digit_cnt_; ++index_oth) {

    for (size_t index = 0; index < digit_cnt_; ++index) {
      result.digits_[index + index_oth] += digits_[index] * other.digits_[index_oth];
    }

    result.updateDigits(index_oth);
  }

  swap(result);
  return *this;
}

BigInteger& BigInteger::operator*=(long long other) {
  if (sign_ == Sign::Zero || other == 0LL) {

    *this = BigInteger();
    return *this;
  }

  for (size_t index = 0; index < digit_cnt_; ++index) {
    digits_[index] *= other;
  }

  updateDigits(0);
  return *this;
}

BigInteger& BigInteger::operator<<(size_t value) {
  if (isZero()) {
    return *this;
  }

  std::vector<long long> new_digits_(value, 0ll);
  digits_.insert(digits_.begin(), new_digits_.begin(), new_digits_.end());
  digit_cnt_ += value;
  return *this;
}

BigInteger& BigInteger::operator/=(const BigInteger& other) {
  if (other.sign_ * sign_ == Sign::Zero) {
    *this = BigInteger();

    if (other.isZero()) {
      std::cerr << "Error: division by zero!\n";
    }

    return *this;
  }

  if (sign_ * other.sign_ == Sign::Positive) {

    if (*this == other) {
      *this = BigInteger(1);
      return *this;
    }

    if ((*this < other) ^ (sign_ == Sign::Negative)) {
      *this = BigInteger();
      return *this;
    }

    BigInteger divisor(other);

    if (sign_ == Sign::Negative) {
      inverse();
      divisor.inverse();
    }

    divisionPositive(*this, divisor);
    return *this;
  }

  inverse();
  *this /= other;
  inverse();

  return *this;
}

BigInteger& BigInteger::operator%=(const BigInteger& other) {
  if (isZero()) {
    return *this;
  }

  bool isNeg = isNegative();
  BigInteger divisor(other);

  if (other.isNegative()) {
    divisor.inverse();
  }

  if (isNeg) {
    inverse();
  }

  if (*this >= other) {
    *this = divisionPositive(*this, divisor);
  }

  if (isNeg) {
    inverse();
  }

  return *this;
}

std::ostream& operator<<(std::ostream& out, const BigInteger& source) {
  out << source.toString();
  return out;
}

std::istream& operator>>(std::istream& in, BigInteger& target) {
  std::string input;
  in >> input;
  target = BigInteger(input);
  return in;
}

bool operator<(const BigInteger& that, const BigInteger& other) {
  if (BigInteger::signProduct(that, other) == Sign::Negative) {
    return that.isNegative();
  }

  size_t digit_cnt_that = that.getDigitCount();
  size_t digit_cnt_other = other.getDigitCount();

  if (digit_cnt_that < digit_cnt_other) {
    return !(that.isNegative());
  }

  if (BigInteger::signProduct(that, other) == Sign::Zero) {
    return that.isZero();
  }

  if (digit_cnt_that > digit_cnt_other) {
    return that.isNegative();
  }

  size_t index = digit_cnt_that - 1;
  const std::vector<long long>& digits_that = that.getDigits();
  const std::vector<long long>& digits_other = other.getDigits();

  while (digits_that[index] == digits_other[index]) {
    if (index == 0) {
      break;
    }
    --index;
  }

  if (digits_that[index] == digits_other[index]) {
    return false;
  }

  return (that.isNegative()) ^ (digits_that[index] < digits_other[index]);
}

bool operator>(const BigInteger& that, const BigInteger& other) {
  return other < that;
}

bool operator>=(const BigInteger& that, const BigInteger& other) {
  return !(that < other);
}

bool operator<=(const BigInteger& that, const BigInteger& other) {
  return !(that > other);
}

bool operator==(const BigInteger& that, const BigInteger& other) {
  if (BigInteger::signProduct(that, other) == Sign::Negative) {
    return false;
  }

  if (BigInteger::signProduct(that, other) == Sign::Zero) {

    return that.isZero() && other.isZero();
  }

  size_t digit_cnt_that = that.getDigitCount();

  if (digit_cnt_that != other.getDigitCount()) {
    return false;
  }

  size_t index = digit_cnt_that - 1;
  const std::vector<long long>& digits_that = that.getDigits();
  const std::vector<long long>& digits_other = other.getDigits();

  while (digits_that[index] == digits_other[index]) {
    if (index == 0) {
      return true;
    }

    --index;
  }

  return false;
}

bool operator!=(const BigInteger& that, const BigInteger& other) {
  return !(that == other);
}

BigInteger operator+(const BigInteger& bi_first, const BigInteger& bi_second) {
  BigInteger result(bi_first);
  result += bi_second;
  return result;
}

BigInteger operator+(int int_first, const BigInteger& bi_second) {
  BigInteger result(bi_second);
  result += static_cast<long long>(int_first);
  return result;
}

BigInteger operator+(const BigInteger& bi_first, int int_second) {
  BigInteger result(bi_first);
  result += static_cast<long long>(int_second);
  return result;
}

BigInteger operator-(const BigInteger& bi_first, const BigInteger& bi_second) {
  BigInteger result(bi_first);
  result -= bi_second;
  return result;
}

BigInteger operator-(int int_first, const BigInteger& bi_second) {
  BigInteger result(bi_second);
  result -= static_cast<long long>(int_first);
  result.inverse();
  return result;
}

BigInteger operator-(const BigInteger& bi_first, int int_second) {
  BigInteger result(bi_first);
  result -= static_cast<long long>(int_second);
  return result;
}

BigInteger operator*(const BigInteger& bi_first, const BigInteger& bi_second) {
  BigInteger result(bi_first);
  result *= bi_second;
  return result;
}

BigInteger operator/(const BigInteger& bi_first, const BigInteger& bi_second) {
  BigInteger result(bi_first);
  result /= bi_second;
  return result;
}

BigInteger operator%(const BigInteger& bi_first, const BigInteger& bi_second) {
  BigInteger result(bi_first);
  result %= bi_second;
  return result;
}

BigInteger operator""_bi(const char* source) {
  std::string temp_input(source);
  BigInteger result(temp_input);
  return result;
}

class Rational;

bool operator==(const Rational& that, const Rational& other);
bool operator<=(const Rational& that, const Rational& other);
bool operator>=(const Rational& that, const Rational& other);
bool operator!=(const Rational& that, const Rational& other);
bool operator<(const Rational& that, const Rational& other);
bool operator>(const Rational& that, const Rational& other);

class Rational {
private:
  Sign sign_;
  BigInteger numerator_;
  BigInteger denominator_;

  void toSimpleFraction() {
    if (numerator_.isZero()) {
      denominator_ = BigInteger(1);
      return;
    }

    BigInteger gcd(BigInteger::gcd(numerator_, denominator_));

    if (gcd != BigInteger(1)) {
      numerator_ /= gcd;
      denominator_ /= gcd;
    }
  }

public:
  static Sign signProduct(const Rational& that, const Rational& other) {
    return that.sign_ * other.sign_;
  }

  ~Rational() = default;

  Rational(): sign_(Sign::Zero), numerator_(BigInteger(0)), denominator_(BigInteger(1)) {}

  Rational(const Rational& source) = default;

  Rational(int source): sign_(Sign::Zero), numerator_(BigInteger(source)),
                              denominator_(BigInteger(1)) {
    if (source < 0) {

      sign_ = Sign::Negative;
      numerator_.inverse();

    } else if (source > 0) {

      sign_ = Sign::Positive;
    }
  }

  Rational(const BigInteger& source): sign_(Sign::Zero), numerator_(source),
                                      denominator_(BigInteger(1)) {
    if (source.isPositive()) {
      sign_ = Sign::Positive;

    } else if (source.isNegative()) {

      sign_ = Sign::Negative;
      numerator_.inverse();
    }
  }

  Rational& operator=(const Rational& other) = default;

  bool isZero() const {
    return sign_ == Sign::Zero;
  }

  bool isNegative() const {
    return sign_ == Sign::Negative;
  }

  bool isPositive() const {
    return sign_ == Sign::Positive;
  }

  void inverse() {
    if (sign_ == Sign::Positive) {

      sign_ = Sign::Negative;

    } else if (sign_ == Sign::Negative) {

      sign_ = Sign::Positive;
    }
  }

  const BigInteger& getNumerator() const {
    return numerator_;
  }

  const BigInteger& getDenominator() const {
    return denominator_;
  }

  Sign getSign() const {
    return sign_;
  }

  Rational operator-() const {
    Rational new_r(*this);
    new_r.inverse();
    return new_r;
  }

  std::string toString() const {
    std::string result;

    if (sign_ == Sign::Zero) {
      result += '0';
      return result;
    }

    if (sign_ == Sign::Negative) {
      result += '-';
    }

    result += numerator_.toString();

    if (denominator_ != BigInteger(1)) {
      result += '/';
      result += denominator_.toString();
    }

    return result;
  }

  std::string asDecimal(size_t precision = 0) const {
    std::string result;

    if (sign_ == Sign::Zero) {
      result += '0';

      if (precision > 0) {
        result += '.';
        result += std::string(precision, '0');
      }

      return result;
    }

    if (sign_ == Sign::Negative) {
      result += '-';
    }

    if (precision == 0) {
      result += (numerator_ / denominator_).toString();
      return result;
    }

    BigInteger numerator_temp(numerator_);
    size_t diff_size = numerator_temp.makeGreaterThan(denominator_);
    size_t shift_size = (precision + BigInteger::base_power) / BigInteger::base_power;

    numerator_temp << shift_size;
    BigInteger::divisionPositive(numerator_temp, denominator_);
    std::string str_temp;

    if (diff_size > 0) {
      result += "0.";
      str_temp += std::string((diff_size - 1) * 9, '0');
      str_temp += numerator_temp.toFullString();
      result += str_temp.substr(0, precision);
      return result;
    }

    str_temp += numerator_temp.toString();
    size_t point_ind = str_temp.size() - shift_size * 9;
    result += str_temp.substr(0, point_ind);
    result += '.';
    result += str_temp.substr(point_ind, precision);

    return result;
  }

  Rational& operator+=(const Rational& other) {
    if (other.isZero()) {
      return *this;
    }

    if (isZero()) {
      *this = other;
      return *this;
    }

    if (sign_ * other.sign_ == Sign::Positive) {
      numerator_ *= other.denominator_;
      numerator_ += denominator_ * other.numerator_;
      denominator_ *= other.denominator_;
      toSimpleFraction();
      return *this;
    }

    inverse();

    if (*this == other) {
      *this = Rational();
      return *this;
    }

    numerator_ = (-numerator_) * other.denominator_ + denominator_ * other.numerator_;
    denominator_ *= other.denominator_;

    if (numerator_.isNegative()) {
      numerator_.inverse();
      inverse();
    }

    toSimpleFraction();
    return *this;
  }

  Rational& operator-=(const Rational& other) {
    inverse();
    *this += other;
    inverse();
    return *this;
  }

  Rational& operator*=(const Rational& other) {
    if (sign_ * other.sign_ == Sign::Zero) {
      *this = Rational();
      return *this;
    }

    sign_ = sign_ * other.sign_;
    numerator_ *= other.numerator_;
    denominator_ *= other.denominator_;
    toSimpleFraction();

    return *this;
  }

  Rational& operator/=(const Rational& other) {
    if (other.isZero()) {
      *this = Rational();
      std::cerr << "Error: division by zero!\n";
      return *this;
    }

    if (isZero()) {
      return *this;
    }

    sign_ = sign_ * other.sign_;
    numerator_ *= other.denominator_;
    denominator_ *= other.numerator_;
    toSimpleFraction();

    return *this;
  }

  explicit operator double() const{
    std::string temp(asDecimal(static_cast<size_t>(310)));
    double result = std::stod(temp);
    return result;
  }
};

bool operator==(const Rational& that, const Rational& other) {
  return ((that.getSign() == other.getSign()) && (that.getNumerator() == other.getNumerator())
                                              && (that.getDenominator() == other.getDenominator()));
}

bool operator<(const Rational& that, const Rational& other) {
  if (Rational::signProduct(that, other) == Sign::Negative || other.isZero()) {
    return that.isNegative();
  }

  if (that.isZero()) {
    return other.isPositive();
  }

  if (that == other) {
    return false;
  }

  return (that.isNegative()) ^ ((that.getNumerator() * other.getDenominator()) < (other.getNumerator() * that.getDenominator()));
}

bool operator>(const Rational& that, const Rational& other) {
  return other < that;
}

bool operator>=(const Rational& that, const Rational& other) {
  return !(that < other);
}

bool operator<=(const Rational& that, const Rational& other) {
  return !(that > other);
}

bool operator!=(const Rational& that, const Rational& other) {
  return !(that == other);
}

Rational operator+(const Rational& first, const Rational& second) {
  Rational result(first);
  result += second;
  return result;
}

Rational operator-(const Rational& first, const Rational& second) {
  Rational result(first);
  result -= second;
  return result;
}

Rational operator*(const Rational& first, const Rational& second) {
  Rational result(first);
  result *= second;
  return result;
}

Rational operator/(const Rational& first, const Rational& second) {
  Rational result(first);
  result /= second;
  return result;
}
